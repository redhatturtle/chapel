class C {
  const x: int;

  proc init(x) {
    this.x = x;
    init this;
    //    this.x = 36;  can't do this
  }
}

class D : C {
  proc init(x) {
    super.init(x);
    init this;
    this.x = 42;   // but can do this...
  }
}

var myD = new shared D(10);
writeln(myD);
