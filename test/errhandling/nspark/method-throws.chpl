use ExampleErrors;
record Wrapper {

  var val: int;
  var err: owned Error? = nil;

  proc ref action() throws {
    if err then throw err;
    writeln(val);
  }

  proc ref oops() {
    err = new owned StringError("called oops()");
  }
}

try {
  var w = new Wrapper(42);
  w.action();
  w.oops();
  w.action();
} catch err {
  writeln(err.message());
}
