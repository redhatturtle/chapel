//
// Check the behavior of PRIM_RESOLVES, a primitive that can be used to
// implement functions in the Reflection module.
//
record r {
  proc doMethodZero() { writeln('zero'); }
  proc doMethodOne(a) { writeln(a); }
  proc doMethodTwo(a, b) {writeln(a, " ", b); }
  proc type doMethodType() { writeln('type'); }
}

proc doProcedureZero() { writeln('zero'); }
proc doProcedureOne(a) { writeln(a); }
proc doProcedureTwo(a, b) { writeln(a, " ", b); }
proc doProcedureParam(param a) { writeln(a); }
proc doProcedureType(type t) { writeln(t); }

proc compilerAssert(param expr: bool) param {
  if !expr then compilerError('Assert failed!', 1);
}

proc test1() {
  param yes = __primitive("resolves", doProcedureZero());
  compilerAssert(yes);
}
test1();

proc test2() {
  param yes1 = __primitive("resolves", doProcedureOne(0));
  compilerAssert(yes1);
  param yes2 = __primitive("resolves", doProcedureOne(a=0));
  compilerAssert(yes2);
  var x = 0;
  param yes3 = __primitive("resolves", doProcedureOne(x));
  compilerAssert(yes3);
  param yes4 = __primitive("resolves", doProcedureOne(a=x));
  compilerAssert(yes4);
  ref y = x;
  param yes5 = __primitive("resolves", doProcedureOne(y));
  compilerAssert(yes5);
  param yes6 = __primitive("resolves", doProcedureOne(a=y));
  compilerAssert(yes6);
  const ref z = x;
  param yes7 = __primitive("resolves", doProcedureOne(z));
  compilerAssert(yes7);
  param yes8 = __primitive("resolves", doProcedureOne(a=z));
  compilerAssert(yes8);
  param no1 = __primitive("resolves", doProcedureOne(foo=z));
  compilerAssert(!no1);
  type T = r;
  param no2 = __primitive("resolves", doProcedureOne(T));
  compilerAssert(!no2);
}
test2();

proc test3() {
  param yes1 = __primitive("resolves", doProcedureTwo(0, new r()));
  compilerAssert(yes1);
}
test3();

proc test4() {

}
test4();

proc test5() {

}
test5();

proc test6() {

}
test6();

proc test7() {

}
test7();

proc test8() {

}
test8();

