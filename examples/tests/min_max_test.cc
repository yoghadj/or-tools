// Copyright 2011-2012 Google
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "base/hash.h"
#include "base/map-util.h"
#include "base/stl_util.h"
#include "base/random.h"
#include "constraint_solver/constraint_solveri.h"
#include "constraint_solver/constraint_solver.h"
#include "util/string_array.h"

namespace operations_research {

class NullDemon : public Demon {
  virtual void Run(Solver* const s) {}
};

// ----- Min Array Test -----

class MinArrayCtTestSetToMin : public DecisionBuilder {
 public:
  MinArrayCtTestSetToMin(IntExpr* const min, const vector<IntVar*>& vars)
      : min_(min), vars_(vars) {}
  virtual ~MinArrayCtTestSetToMin() {}

  virtual Decision* Next(Solver* const s) {
    min_->SetMax(0);
    CHECK(vars_[0]->Bound()) << "var not bound";
    CHECK_EQ(0, vars_[0]->Min()) << "var not bound to the correct value";
    return NULL;
  }

 private:
  IntExpr* const min_;
  const vector<IntVar*>& vars_;
};

class MinArrayCtTestSetToMax : public DecisionBuilder {
 public:
  MinArrayCtTestSetToMax(IntExpr* const min, const vector<IntVar*>& vars)
      : min_(min), vars_(vars) {}
  virtual ~MinArrayCtTestSetToMax() {}

  virtual Decision* Next(Solver* const s) {
    min_->SetMin(5);
    CHECK(vars_[0]->Bound()) << "var not bound";
    CHECK_EQ(5, vars_[0]->Min()) << "var not bound to the correct value";
    return NULL;
  }

 private:
  IntExpr* const min_;
  const vector<IntVar*>& vars_;
};

class MinArrayCtTestSetOneVar : public DecisionBuilder {
 public:
  MinArrayCtTestSetOneVar(IntExpr* const min, const vector<IntVar*>& vars)
      : min_(min), vars_(vars) {}
  virtual ~MinArrayCtTestSetOneVar() {}

  virtual Decision* Next(Solver* const s) {
    vars_[0]->SetValue(5);
    CHECK_EQ(1, min_->Min()) << "bad computed min in min_array";
    CHECK_EQ(5, min_->Max()) << "bad computed max in min_array";
    return NULL;
  }

 private:
  IntExpr* const min_;
  const vector<IntVar*>& vars_;
};

class MinArrayCtTest {
 public:
  void SetUp() {
    solver_.reset(new Solver("MinConstraintTest"));
    vars_.clear();
    vars_.resize(10);
    for (int i = 0; i < 10; ++i) {
      vars_[i] = solver_->MakeIntVar(i, 2 * i + 5);
    }
    min_ = solver_->MakeMin(vars_)->Var();
  }

  scoped_ptr<Solver> solver_;
  vector<IntVar*> vars_;
  IntExpr* min_;

  void TestAlternateCtor() {
    SetUp();
    scoped_array<IntVar*> vars(new IntVar*[4]);
    for (int i = 0; i < 4; ++i) {
      vars[i] = solver_->MakeIntVar(i, 2 * i);
    }
    IntExpr* emin = solver_->MakeMin(vars.get(), 4);
    CHECK(!emin->DebugString().empty());
  }

  void TestBounds() {
    SetUp();
    CHECK_EQ(0LL, min_->Min()) << "bad computed min in min_array";
    CHECK_EQ(5, min_->Max()) << "bad computed min in min_array";
  }

  void TestSetToMin() {
    SetUp();
    solver_->Solve(solver_->RevAlloc(new MinArrayCtTestSetToMin(min_, vars_)));
  }

  void TestSetToMax() {
    SetUp();
    solver_->Solve(solver_->RevAlloc(new MinArrayCtTestSetToMax(min_, vars_)));
  }

  void TestSetOneVar() {
    SetUp();
    solver_->Solve(solver_->RevAlloc(new MinArrayCtTestSetOneVar(min_, vars_)));
  }

  void TestWhen() {
    SetUp();
    Demon* const d = solver_->RevAlloc(new NullDemon());
    min_->WhenRange(d);
  }

  void TestBigMinVector() {
    SetUp();
    vector<IntVar*> vars;
    for (int i = 0; i < 1001; ++i) {
      vars.push_back(solver_->MakeIntVar(i, 3000 - i, StringPrintf("x%d", i)));
    }
    IntExpr* expr = solver_->MakeMin(vars);
    CHECK_EQ(2000, expr->Max());
    CHECK_EQ(0, expr->Min());
  }

  void TestBigMinArray() {
    SetUp();
    scoped_array<IntVar*> vars(new IntVar*[1001]);
    for (int i = 0; i < 1001; ++i) {
      vars[i] = solver_->MakeIntVar(i, 3000 - i, StringPrintf("x%d", i));
    }
    IntExpr* expr = solver_->MakeMin(vars.get(), 1001);
    CHECK_EQ(2000, expr->Max());
    CHECK_EQ(0, expr->Min());
  }

  void TestSmallMinVector() {
    SetUp();
    vector<IntVar*> vars;
    IntExpr* expr = solver_->MakeMin(vars);
    CHECK_EQ(0, expr->Min());
    CHECK_EQ(0, expr->Max());
    vars.push_back(solver_->MakeIntVar(1, 10, "x0"));
    expr = solver_->MakeMin(vars);
    CHECK_EQ(1, expr->Min());
    CHECK_EQ(10, expr->Max());
    vars.push_back(solver_->MakeIntVar(2, 9, "x1"));
    expr = solver_->MakeMin(vars);
    CHECK_EQ(1, expr->Min());
    CHECK_EQ(9, expr->Max());
    vars.push_back(solver_->MakeIntVar(3, 8, "x2"));
    expr = solver_->MakeMin(vars);
    CHECK_EQ(1, expr->Min());
    CHECK_EQ(8, expr->Max());
  }

  void TestSmallMinArray() {
    SetUp();
    scoped_array<IntVar*> vars(new IntVar*[3]);
    for (int i = 0; i < 3; ++i) {
      vars[i] = solver_->MakeIntVar(i + 1, 10 - i, StringPrintf("x%d", i));
    }
    IntExpr* expr = solver_->MakeMin(vars.get(), 0);
    CHECK_EQ(0, expr->Min());
    CHECK_EQ(0, expr->Max());
    expr = solver_->MakeMin(vars.get(), 1);
    CHECK_EQ(1, expr->Min());
    CHECK_EQ(10, expr->Max());
    expr = solver_->MakeMin(vars.get(), 2);
    CHECK_EQ(1, expr->Min());
    CHECK_EQ(9, expr->Max());
    expr = solver_->MakeMin(vars.get(), 3);
    CHECK_EQ(1, expr->Min());
    CHECK_EQ(8, expr->Max());
  }
};

// ----- Max Array Test -----

class MaxArrayCtTestSetToMin : public DecisionBuilder {
 public:
  MaxArrayCtTestSetToMin(IntExpr* const max, const vector<IntVar*>& vars)
      : max_(max), vars_(vars) {}
  virtual ~MaxArrayCtTestSetToMin() {}

  virtual Decision* Next(Solver* const s) {
    max_->SetMin(23);
    CHECK(vars_[9]->Bound()) << "var not bound";
    CHECK_EQ(23, vars_[9]->Min()) << "var not bound to the correct value";
    return NULL;
  }

 private:
  IntExpr* const max_;
  const vector<IntVar*>& vars_;
};

class MaxArrayCtTestSetToMax : public DecisionBuilder {
 public:
  MaxArrayCtTestSetToMax(IntExpr* const max, const vector<IntVar*>& vars)
      : max_(max), vars_(vars) {}
  virtual ~MaxArrayCtTestSetToMax() {}

  virtual Decision* Next(Solver* const s) {
    max_->SetMax(9);
    CHECK(vars_[9]->Bound()) << "var not bound";
    CHECK_EQ(9, vars_[9]->Min()) << "var not bound to the correct value";
    return NULL;
  }

 private:
  IntExpr* const max_;
  const vector<IntVar*>& vars_;
};

class MaxArrayCtTestSetOneVar : public DecisionBuilder {
 public:
  MaxArrayCtTestSetOneVar(IntExpr* const max, const vector<IntVar*>& vars)
      : max_(max), vars_(vars) {}
  virtual ~MaxArrayCtTestSetOneVar() {}

  virtual Decision* Next(Solver* const s) {
    vars_[9]->SetValue(18);
    CHECK_EQ(18, max_->Min()) << "bad computed min in max_array";
    CHECK_EQ(21, max_->Max()) << "bad computed max in max_array";
    return NULL;
  }

 private:
  IntExpr* const max_;
  const vector<IntVar*>& vars_;
};

class MaxArrayCtTest {
 public:
  void SetUp() {
    solver_.reset(new Solver("MaxArrayCtTest"));
    vars_.resize(10);
    for (int i = 0; i < 10; ++i) {
      vars_[i] = solver_->MakeIntVar(i, 2 * i + 5);
    }
    max_ = solver_->MakeMax(vars_)->Var();
  }

  scoped_ptr<Solver> solver_;
  vector<IntVar*> vars_;
  IntExpr* max_;

  void TestAlternateCtor() {
    SetUp();
    scoped_array<IntVar*> vars(new IntVar*[4]);
    for (int i = 0; i < 4; ++i) {
      vars[i] = solver_->MakeIntVar(i, 2 * i);
    }
    IntExpr* emax = solver_->MakeMax(vars.get(), 4);
    CHECK(!emax->DebugString().empty());
  }

  void TestBounds() {
    SetUp();
    CHECK_EQ(9, max_->Min()) << "bad computed min in max_array";
    CHECK_EQ(23, max_->Max()) << "bad computed min in max_array";
  }

  void TestSetToMin() {
    SetUp();
    solver_->Solve(solver_->RevAlloc(new MaxArrayCtTestSetToMin(max_, vars_)));
  }

  void TestSetToMax() {
    SetUp();
    solver_->Solve(solver_->RevAlloc(new MaxArrayCtTestSetToMax(max_, vars_)));
  }

  void TestSetOneVar() {
    SetUp();
    solver_->Solve(solver_->RevAlloc(new MaxArrayCtTestSetOneVar(max_, vars_)));
  }

  void TestWhen() {
    SetUp();
    Demon* d = solver_->RevAlloc(new NullDemon());
    max_->WhenRange(d);
  }

  void TestBigMaxVector() {
    SetUp();
    vector<IntVar*> vars;
    for (int i = 0; i < 1001; ++i) {
      vars.push_back(solver_->MakeIntVar(i, 3000 - i, StringPrintf("x%d", i)));
    }
    IntExpr* expr = solver_->MakeMax(vars);
    CHECK_EQ(3000, expr->Max());
    CHECK_EQ(1000, expr->Min());
  }

  void TestBigMaxArray() {
    SetUp();
    scoped_array<IntVar*> vars(new IntVar*[1001]);
    for (int i = 0; i < 1001; ++i) {
      vars[i] = solver_->MakeIntVar(i, 3000 - i, StringPrintf("x%d", i));
    }
    IntExpr* expr = solver_->MakeMax(vars.get(), 1001);
    CHECK_EQ(3000, expr->Max());
    CHECK_EQ(1000, expr->Min());
  }

  void TestSmallMaxVector() {
    SetUp();
    vector<IntVar*> vars;
    IntExpr* expr = solver_->MakeMax(vars);
    CHECK_EQ(0, expr->Min());
    CHECK_EQ(0, expr->Max());
    vars.push_back(solver_->MakeIntVar(1, 10, "x0"));
    expr = solver_->MakeMax(vars);
    CHECK_EQ(1, expr->Min());
    CHECK_EQ(10, expr->Max());
    vars.push_back(solver_->MakeIntVar(2, 9, "x1"));
    expr = solver_->MakeMax(vars);
    CHECK_EQ(2, expr->Min());
    CHECK_EQ(10, expr->Max());
    vars.push_back(solver_->MakeIntVar(3, 8, "x2"));
    expr = solver_->MakeMax(vars);
    CHECK_EQ(3, expr->Min());
    CHECK_EQ(10, expr->Max());
  }

  void TestSmallMaxArray() {
    SetUp();
    scoped_array<IntVar*> vars(new IntVar*[3]);
    for (int i = 0; i < 3; ++i) {
      vars[i] = solver_->MakeIntVar(i + 1, 10 - i, StringPrintf("x%d", i));
    }
    IntExpr* expr = solver_->MakeMax(vars.get(), 0);
    CHECK_EQ(0, expr->Min());
    CHECK_EQ(0, expr->Max());
    expr = solver_->MakeMax(vars.get(), 1);
    CHECK_EQ(1, expr->Min());
    CHECK_EQ(10, expr->Max());
    expr = solver_->MakeMax(vars.get(), 2);
    CHECK_EQ(2, expr->Min());
    CHECK_EQ(10, expr->Max());
    expr = solver_->MakeMax(vars.get(), 3);
    CHECK_EQ(3, expr->Min());
    CHECK_EQ(10, expr->Max());
  }
};
}  // namespace operations_research

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  operations_research::MinArrayCtTest min_test;
  min_test.TestAlternateCtor();
  min_test.TestBounds();
  min_test.TestSetToMin();
  min_test.TestSetToMax();
  min_test.TestSetOneVar();
  min_test.TestWhen();
  min_test.TestBigMinVector();
  min_test.TestBigMinArray();
  min_test.TestSmallMinVector();
  min_test.TestSmallMinArray();

  operations_research::MaxArrayCtTest max_test;
  max_test.TestAlternateCtor();
  max_test.TestBounds();
  max_test.TestSetToMin();
  max_test.TestSetToMax();
  max_test.TestSetOneVar();
  max_test.TestWhen();
  max_test.TestBigMaxVector();
  max_test.TestBigMaxArray();
  max_test.TestSmallMaxVector();
  max_test.TestSmallMaxArray();

  return 0;
}


