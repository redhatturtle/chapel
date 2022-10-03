use IO;
use OS;
use CTypes;

// array is too small to fit full file...
var a : [0..<100] uint(8),
    num_b: int = 0;

var ch = openreader("./jab.txt");
try {
    num_b = ch.readAll(a);
} catch e:IoError {
    // correct error was thrown:
    writeln(e);
} catch {
    writeln("threw wrong error type");
}

// num_b is still zero, as the method did not return:
writeln(num_b == 0);

// can still continue reading the file after catching error:
const s_remaining = ch.readAll(string);
ch.close();

// the concatenation of the first 100 bytes, and the remaining bytes recreates the whole file:
const s_from_array = createStringWithBorrowedBuffer(c_ptrTo(a), length=a.size, size=a.size);
writeln(s_from_array + s_remaining);
