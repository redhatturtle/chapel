config param printTimes = false;
config param maxAdds = 5;
config const size = 100000;
config const nTrials = 100;

proc arrayAccess(A: [] real, ref B: [] real, param nAdds) {
  const n = A.size;
  for 1..nTrials {
    forall i in 0..#n with (ref B) {
      for param adds in 1..nAdds {
        B[i] += A[i] + A[i];
      }
    }
  }
}

proc refAccess(ref A: [] real, ref B: [] real, param nAdds) {
  const n = A.size;
  for 1..nTrials {
    forall i in 0..#n with (ref A, ref B) {
      ref rA = A[i];
      for param adds in 1..nAdds {
        B[i] += rA + rA;
      }
    }
  }
}

proc main {
  use Time;
  var t1, t2: stopwatch;
  var A: [0..#size] real;
  var B: [0..#size] real;
  var C: [0..#size] real;

  for i in 0..#size do A[i] = i * 0.123456789;

  t1.start();
  arrayAccess(A, B, maxAdds);
  t1.stop();

  t2.start();
  refAccess(A, C, maxAdds);
  t2.stop();

  for i in 0..#size do
    if B[i] != C[i] then
      halt("At index ", i, ": ", B[i], " != ", C[i]);

  if printTimes {
    writeln("Array access: ", t1.elapsed());
    writeln("Ref access: ", t2.elapsed());
  }
}

