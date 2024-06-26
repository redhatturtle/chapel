config const n = 5;
config const m = 5;
config const initialize = true;
config const printOutput = true;
config const printTiming = false;
config const doCommDiag = false;

use CommDiagnostics;
use Time;

var l: [1..numLocales] sync bool;
var A: [1..n,1..m] real;

writeln("n=", n, " m=", m);

enum testTypes {initial, lhs, rhs, both};

proc dit (ref A, param ttype: testTypes) {
  // This is a bit of a hack, we used to have this enum value as 'init' back
  // before that was a Chapel keyword.  We use the names of these enum values
  // as "perfkeys".  To avoid disrupting the "perfkeys" we will continue to use
  // 'init' as the string representation for initial. If this really bothers
  // anyone we could go about properly changing the perfkey label by using the
  // ` util/devel/updateDatFiles.py` script.
  var ttypeStr = ttype:string;
  if ttypeStr == "initial" then ttypeStr = "init";

  for loc in Locales do
    on loc {
      var B: [1..n,1..m] real;
      if ttype != testTypes.initial then B = loc.id+1;
      if loc.id != 0 then
        l[loc.id].readFE();
      if doCommDiag then startCommDiagnostics();
      var st = timeSinceEpoch().totalSeconds();
      select ttype {
        when testTypes.initial do A = loc.id+1;
        when testTypes.lhs do A = B;
        when testTypes.rhs do B = A;
        when testTypes.both do compilerError("Both is stupid.\n");
        }
      var dt = timeSinceEpoch().totalSeconds()-st;
      if doCommDiag then stopCommDiagnostics();
      if printOutput {
        writeln("Remote ", ttypeStr, " (Locale ", loc.id, "):");
        writeln("A:");
        on Locales(0) do writeln(A);
        writeln("B:");
        writeln(B);
      }
      if doCommDiag {
        var Diagnostics = getCommDiagnostics();
        writeln("Remote ", ttypeStr, " (Locale ", loc.id,
                "): (gets, puts, forks, fast forks, non-blocking forks)");
        for (diagnostics, lid) in zip(Diagnostics,1..) do
          writeln(lid, ": ", diagnostics);
      }
      if printTiming {
        writeln("Remote ", ttypeStr, " (Locale ", loc.id, "): ", dt);
      }
      if loc.id != numLocales-1 then
        l[loc.id+1].writeEF(true);
    }
}

A = -1;
dit(A, testTypes.initial);

A = -1;
dit(A, testTypes.lhs);

for (i,j) in {1..n,1..m} do A(i,j) = (i-1)*m+j;
dit(A, testTypes.rhs);
