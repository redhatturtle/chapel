import Set.set;

var gid = 0;
proc makeId() { var result = gid; gid += 1; return result; }

record r { var _id: int = makeId(); }

proc r.init() {
  init this;
  writeln('init r: ', _id);
}

proc r.init=(other: r) {
  init this;
  writeln('init r: ', _id, ' from r: ', other._id);
}

proc r.deinit() {
  writeln('deinit r: ', _id);
}

operator r.=(ref lhs: r, const ref rhs: r) {
  writeln('assigning r: ', lhs._id, ' from r: ', rhs._id);
}

proc test() {
  var s: set(r);
  var foo = new r();
  s.add(foo);
  s.remove(foo);
}
test();

