use Time;
use ResultDB;
use IO.FormattedIO;
use GPUDiagnostics;

config const passes = 10;
config const alpha = 1.75: real(32);
config const noisy = false;
config var tolerance = 1;
config const output = true;


proc main(){
    var timer: Timer;
    var kernelTimer: Timer;

    // Numbers just copied from cuda version
    // 256K through 8M bytes. (scaled down by 1024)
    const blockSizes = [64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384];
    param maxProblemSize = 16384;
    param numMaxFloats = 1024 * maxProblemSize / numBytes(real(32)); // 4 is the size of real(32)
    param halfNumFloats = numMaxFloats/2;

    var hos: [0..#numMaxFloats] real(32);
    startGPUDiagnostics();
    on here.gpus[0] {
        var flopsDB = new ResultDatabase("TriadFlops", "GFLOP/s");
        var bdwthDB = new ResultDatabase("TriadBdwth", "GB/s");
        var triadDB = new ResultDatabase("Triad Time", "sec");
        var kernelDB = new ResultDatabase("Kernel Time", "sec");
        var kernelLaunches = 0;
        for pass in 0..#passes{
            for blkSize in blockSizes {
                // Populate host with arbitrary memory
                // TODO: Change arbitrary to random
                for i in 0..#halfNumFloats {
                    hos[i] = i:int(32)%16:int(32) + 0.12:real(32);
                    hos[halfNumFloats+i] = hos[i] ;

                }
                // Number of floats that fit in the number of bytes denoted
                // by problem size. Converting MB to Bytes and dividing by
                // size of real(32)
                var elemsInBlock = blkSize * 1024 / numBytes(real(32));
                // Make A's
                var A0 : [0..#elemsInBlock] real(32);
                var A1 : [0..#elemsInBlock] real(32);

                // ... Make B's and C's..
                var B0 : [0..#elemsInBlock] real(32);
                var B1 : [0..#elemsInBlock] real(32);

                var C0 : [0..#elemsInBlock] real(32);
                var C1 : [0..#elemsInBlock] real(32);

                if noisy then
                writeln(" >>> Executing Triad with arrays of length ", numMaxFloats,
                        " and block size of ", elemsInBlock, " elements");

                timer.start();

                // Do first transfer for A and B
                A0= hos[0..#elemsInBlock];
                B0= hos[0..#elemsInBlock];

                // Do the calculation
                kernelLaunches+=1;
                kernelTimer.start();
                forall i in 0..#elemsInBlock {
                    C0[i] = A0[i] * alpha + B0[i];
                }
                kernelTimer.stop();

                if(elemsInBlock < numMaxFloats){
                    // Transfers for other stream
                    // In theory these memory transfers could be
                    // asynchronous until we get to the second kernel launch
                    A1= hos[elemsInBlock..#elemsInBlock];
                    B1= hos[elemsInBlock..#elemsInBlock];
                }


                var currentStream = true; // Representing Stream 1
                                        // false represents stream 0
                var currentIdx = 0;
                // Loop corresponding to while loop in benchmark:
                while(currentIdx < numMaxFloats) {
                    // Copy back answer from last kernel call
                    if(currentStream){
                        // Stream 0 copy back
                        hos[currentIdx..#elemsInBlock] = C0;
                    } else {
                        // Stream 1 copy back
                        hos[currentIdx..#elemsInBlock] = C1;
                    }

                    currentIdx += elemsInBlock;

                    if(currentIdx < numMaxFloats){
                        kernelLaunches+=1;
                        // We have more stuff to operate on
                        if(currentStream){
                            kernelTimer.start();
                            // Launch on Stream 1
                            forall i in 0..#elemsInBlock {
                                C1[i] = A1[i] * alpha + B1[i];
                            }
                            kernelTimer.stop();
                        } else {
                            kernelTimer.start();
                            // Launch on Stream 0
                            forall i in 0..#elemsInBlock {
                                C0[i] = A0[i] * alpha + B0[i];
                            }
                            kernelTimer.stop();
                        }
                    }

                    if(currentIdx+elemsInBlock < numMaxFloats){
                        // We have more stuff to copy over and then operate on
                        // Copy back answer from last kernel call
                        if(currentStream){
                            // Stream 1 copy over
                            A1 = hos[currentIdx+elemsInBlock..#elemsInBlock];
                            B1 = hos[currentIdx+elemsInBlock..#elemsInBlock];
                        } else {
                            // Stream 0 copy over
                            A0 = hos[currentIdx+elemsInBlock..#elemsInBlock];
                            B0 = hos[currentIdx+elemsInBlock..#elemsInBlock];
                        }
                    }

                    currentStream  = !currentStream;
                }

                timer.stop();
                var timeElapsedInNanoseconds = timer.elapsed() * 1e9;
                var triad = (numMaxFloats * 2.0) / timeElapsedInNanoseconds;
                var bdwth = (numMaxFloats * numBytes(real(32)) * 3.0) // 4 is size of real(32)
                /timeElapsedInNanoseconds;

                if noisy {
                    writeln("TriadFlops\t",
                    // "Block:%{#####}KB\t".format(blkSize),
                    "Block",blkSize, "KB\t",
                    "GFLOP/s\t",
                    triad);

                    writeln("TriadBdwth\t",
                    // "Block:%{#####}KB\t".format(blkSize),
                    "Block",blkSize, "KB\t",
                    "GB/s\t",
                    bdwth);

                    writeln("Triad Time : ", timer.elapsed());
                    writeln("Kernel Time : ", kernelTimer.elapsed());
                    writeln("Kernel Launches : ", kernelLaunches);
                }

                flopsDB.addToDatabase("%{#####}".format(blkSize), triad);
                bdwthDB.addToDatabase("%{#####}".format(blkSize), bdwth);
                triadDB.addToDatabase("%{#####}".format(blkSize), timer.elapsed());
                kernelDB.addToDatabase("%{#####}".format(blkSize), kernelTimer.elapsed());
                kernelLaunches = 0;
                kernelTimer.clear();
                timer.clear();
                // Error Checking for the correct anwer
                // Since we gave the kernel arbitrary numbers we know what the answer
                // to the computation is
                // The formula used to calculate the index i on hos was
                // hos[i] = i%16 + 0.12;
                // Therefore the numbers in host repeat the following sequence over
                // and over: 0.12 1.12 2.12 3.12 4.12 5.12 6.12 7.12 8.12 9.12 10.12 11.12 12.12. 13.12. 14.12 15.12
                // The computation was
                // hos[i] = hos[i] * alpha + hos[i]
                // Therefore the answers should be equal to that
                // Each hos[i] after the computation is therefore as follows
                // hos[i] = (i%10+0.12) * alpha + (i%10+0.12)
                // The first expected answer, for example, will then be 0.12*1.75 + 0.12 = 0.33
                // Remember this was only for half of the memory.
                // The other half was just a copy of the first half
                // Check that on the CPU instead of the GPU
                for i in 0..#halfNumFloats {
                    var expected:real(32) = (i:int(32)%16:int(32) + 0.12:real(32)) * alpha + (i:int(32)%16:int(32) + 0.12:real(32));
                    var actual = hos[i];
                    // if(!isclose(actual, expected)) {
                    //     writeln("Pass: ", pass,
                    //     "\nBlock Size:", blkSize,
                    //     "\nIndex: ", i,
                    //     "\nExpected: ", expected, "\nActual: ", actual);
                    //     writeln(isclose(actual, expected));
                    //     // tolerance-=1;
                    //     // if(tolerance==0) then
                    //     break;
                    // }
                    assert(isclose(hos[i] , expected));
                    assert(hos[halfNumFloats+i] == hos[i]);
                }
            }
        }
        if(output) {
            flopsDB.printDatabaseStats();
            bdwthDB.printDatabaseStats();
            triadDB.printDatabaseStats();
            kernelDB.printDatabaseStats();
        }
    }
    stopGPUDiagnostics();
    writeln(getGPUDiagnostics());
}
