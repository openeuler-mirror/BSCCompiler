/*
* Copyright (C) [2021-2022] Futurewei Technologies, Inc. All rights reverved.
*
* OpenArkFE is licensed under the Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*
*  http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
* FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/
#include "helper.h"

namespace maplefe {

FuncTable hFuncTable;
GeneratorLabels GenFnLabels;

std::unordered_map<TypeId, std::string>TypeIdToJSTypeCXX = {
  // AST TypeId to t2crt JS_Type mapping for JS_Val type of obj props that pts to CXX class fields 
  {TY_Object,  "t2crt::TY_CXX_Object"},
  {TY_Function,"t2crt::TY_CXX_Function"},
  {TY_Boolean, "t2crt::TY_CXX_Bool"},
  {TY_Int,     "t2crt::TY_CXX_Long"},
  {TY_String,  "t2crt::TY_CXX_String"},
  {TY_Number,  "t2crt::TY_CXX_Double"},
  {TY_Double,  "t2crt::TY_CXX_Double"},
  {TY_Array,   "t2crt::TY_CXX_Object"},
  {TY_Class,   "t2crt::TY_CXX_Object"},
  {TY_Any,     "t2crt::TY_CXX_Any"},
};

std::string GeneratorFn_start = R"""(
  if (yield != nullptr)
    goto *yield;
)""";

std::string GeneratorFn_return = R"""(
  res.value = undefined;
  res.done  = true;
  return;
)""";

// Used to build GetProp<xxx> for calls to get Object (class Object in ts2cpp.h) property
std::string hlpGetJSValTypeStr(TypeId typeId) {
  switch(typeId) {
    case TY_Object:
    case TY_Class:
      return "Obj";
    case TY_Any:
      return "";
    case TY_Function:
      return "Func";
    case TY_Boolean:
      return "bool";
    case TY_Number:
      return "Double";
    case TY_String:
      return "Str";
  }
  return std::string();
}

// Get TypeId info for a treenode.
TypeId hlpGetTypeId(TreeNode* node) {
  // lookup typetable
  if (node->GetTypeIdx()) {
    TypeEntry* te = gTypeTable.GetTypeEntryFromTypeIdx(node->GetTypeIdx());
    if (te) 
      return te->GetTypeId();
  }

  // lookup ast node mTypeId (will be deprecated)
  if (node->GetTypeId() != TY_None) {
    return node->GetTypeId();
  }
  return TY_Any;
}

// Temp. workaround to get func info for var of type function. TODO: replace with TI API when avail.
std::string GetClassOfAssignedFunc(TreeNode* node) {
  if (node->IsIdentifier() &&
      node->GetParent() &&
      node->GetParent()->IsDecl() &&
      static_cast<DeclNode*>(node->GetParent())->GetInit() &&
      static_cast<DeclNode*>(node->GetParent())->GetInit()->IsFunction()) {
    auto n = static_cast<DeclNode*>(node->GetParent())->GetInit();
    return("Cls_"s + static_cast<FunctionNode*>(n)->GetName());
  }
  return std::string();
}

// Generate call to create obj prop with ptr to c++ class fld member
// e.g. obj->AddProp("fdLong", t2crt::ClassFld<long   Foo::*>(&Foo::fdLong).NewProp(this, t2crt::TY_CXX_Long))
//      obj->AddProp("fdAny",  t2crt::ClassFld<JS_Val Foo::*>(&Foo::fdAny).NewProp(this,  t2crt::TY_CXX_Any))
std::string GenClassFldAddProp(std::string objName,
                               std::string clsName,
                               std::string fldName,
                               std::string fldCType,
                               std::string fldJSType) {
  std::string str;
  str = objName + "->AddProp(\"" + fldName + "\", t2crt::ClassFld<" + 
    fldCType + " " + clsName + "::*>(&" +
    clsName + "::" + fldName + ").NewProp(this, " + fldJSType + "))";
  return str;
}

// From TS func param info, generate param and arg list for corresponding mapped C++ func.
//
// Different formats of arg list as needed by C++ mapping of function/class/generators
// <type>    <argName>   - args for function class functor and generation class constructor
// <argName> (<ArgName>) - generator class constructor field init list
// <type>&   <argName>   - args passed by reference to generation function _body method
// <type>    <argName>;  - generator class fields for capturing closure
//
std::string FunctionParams(unsigned nodeId, bool handleThis, bool argsOnly, bool byRef, bool fdInit, bool capture) {
  std::vector<std::pair<std::string, std::string>> funcParams = hFuncTable.GetArgInfo(nodeId);
  std::string ObjT = "t2crt::Object*";
  std::string str;

  // "this" in TS function paramter mapping to C++:
  //
  // TS2cpp's C++ mapping for TS func has a "this" obj in the c++ func param list
  // which will be generated from AST if "this" is declared as a TS func parameter
  // as required by TS strict mode. However TS funcs that do not reference 'this'
  // are not required to declare it, in which case emitter has to insert one.
  //
  // Cases:
  // if TS func has no param
  //   - insert param "ts2crt::Object* _this"
  // if 1st TS func param is not "this"
  //   - insert param "ts2crt::Object* _this"
  // if 1st TS func param is "this"
  //   - rename to "_this"
  //   - if type is Any (JS_Val), change to "ts2crt::Object*"
  //
  if (handleThis) {
    if (funcParams.size() == 0)          // func has no param
      return argsOnly ? "_this"s : (ObjT + " _this");
  }

  for (bool first=true; auto elem : funcParams) {
    std::string type = elem.first, name = elem.second;
    if (!first)
      str += ", "s;
    else {                              // 1st param of TS func
      if (handleThis) {
        if (name.compare("this") != 0)  // if not "this", insert _this
          str += argsOnly? ("_this, "s): (ObjT + " _this, "s);
        else {                          // if "this"
          name = "_this";               // rename to "_this"
          if (type.compare("t2crt::JS_Val") == 0)
            type = ObjT;                // change type Any to Object*
        }
      }
      first = false;
    }
    if (fdInit)
      str += name + "(" + name + ")";
    else if (capture)
      str += type + " " + name + ";\n";
    else
      str += argsOnly? name: (type + (byRef?"\&":"") +  " "s + name);
  }
  return str;
}

// Each first level function is instantiated from a corresponding class generated with interfaces below:
//   Body  - user defined function code
//   ()    - functor for OrdinaryCallEvaluteBody [9.2.1.3]
//   Ctor  - Call user defined code as constructor (with object from new() op)
//   Call  - Call user defined code with designated 'this'
//   Apply - Call user defined code with designated 'this' and array of argument
//   Bind  - Create and return function object binded to designated this and optional args
// note: the parameter args is expected to be a string that start with "_this"
//       TODO: apply and bind may be moved to ts2cpp.h Fuction class as virtual
// note: TSC prohibits calling non-void constructor func with new(), so in the code generated below
//       for ctor(), it calls _body() but ignores return val from _body(), and instead returns _this
//       per TS/JS spec.

std::string FunctionClassDecl(std::string retType, std::string funcName, unsigned nodeId) {
  std::string str, args, params, thisType;

  std::string clsName = ClsName(funcName);
  params = FunctionParams(nodeId, true, false);
  args   = FunctionParams(nodeId, true, true);
  thisType = params.substr(0, params.find(" "));  // extract return type of "this" parameter

  std::string functorParams = params;
  std::string functorArgs   = args;
  functorArgs.replace(0, 5, "_thisArg"); // replace _this with _thisArg
  size_t pos;
  if ((pos = functorParams.find("_this, ")) != std::string::npos)
    functorParams.erase(0, pos+7);
  else if ((pos = functorParams.find("_this")) != std::string::npos)
    functorParams.erase(0, pos+5);

  str = R"""(
class )""" + clsName + R"""( : public t2crt::Function {
  public:
    )""" + clsName + R"""(() : t2crt::Function(&t2crt::Function::ctor,t2crt::Function::ctor.prototype,t2crt::Object::ctor.prototype) {}
    ~)""" + clsName + R"""(() {}

    )""" + retType + R"""(  _body()""" + params + R"""();
    )""" + retType + R"""(  operator()()""" + functorParams + R"""() { return _body(()""" + thisType + R"""())""" + functorArgs + R"""(); }
    t2crt::Object*  ctor ()""" + params + R"""() { _body()""" + args + R"""(); return(_this); }
    )""" + retType + R"""(  call ()""" + params + R"""() { return _body()""" + args + R"""( ); }
    )""" + retType + R"""(  apply(t2crt::Object* _this, t2crt::ArgsT& args) { /* TODO: call _body wtih flatten args  */ }
    )""" + clsName + R"""(* bind (t2crt::Object* _this, t2crt::ArgsT* args) {
      )""" + clsName + R"""(* func = new )""" + clsName + R"""(();
      func->_thisArg = _this;
      func->_args = args;
      return(func);
    }
    virtual const char* __GetClassName() const {return ")""" + funcName + R"""( ";}
};

)""";
  return str;
}

// build generator function header for _body
std::string GeneratorFuncHeader(std::string cls, unsigned nodeId) {
  std::string params = FunctionParams(nodeId, false, false, true); // pass params by ref into _body()
  if (!params.empty())
    params = ", " + params;
  return "void " + cls + "_body(t2crt::Object* _this, void*& yield, t2crt::IteratorResult& res" + params + ")";
}

// Generating Generators and Generator Functions:
// For each TS generator function, 2 C++ classes: generator and generator function are emitted.
// The generator function has only a single instance. It is called to create generator instances.
std::string GeneratorClassDecl(std::string funcName, unsigned nodeId) {
  std::string str;
  std::string generatorName = GeneratorName(funcName);
  std::string generatorFuncName = GeneratorFuncName(funcName);

  std::string functorArgs  = FunctionParams(nodeId, false, false);
  std::string initList     = FunctionParams(nodeId, false, false, false, true) ;
  std::string captureFields = FunctionParams(nodeId, false, false, false, false, true);
  std::string ctorArgs = functorArgs.empty()? std::string(): (", "s + functorArgs);
  initList = initList.empty()? "": (", "s + initList);

  std::string genClsDecl[] = {
"// " +funcName+ " generators",
"class " +generatorName+ " : public t2crt::GeneratorProto {",
"public:",
"  " +generatorName+ "(t2crt::Function* ctor, t2crt::Object* proto" +ctorArgs+ ") : t2crt::GeneratorProto(ctor, proto)" +initList+ " {}",
"  ~" +generatorName+ "() {}",
"  // closure capture fields",
"  " +captureFields,
"  // iterator interface (override _return and _throw when needed)",
"  t2crt::IteratorResult* next(t2crt::JS_Val* arg = nullptr) override;",
"};",
"// " +funcName+ " generator function",
"class " +generatorFuncName+ " : public t2crt::GeneratorFuncPrototype {",
"public:",
"  " +generatorFuncName+ "() : t2crt::GeneratorFuncPrototype(&t2crt::GeneratorFunction, &t2crt::Generator, t2crt::GeneratorPrototype) {}",
"  ~" +generatorFuncName+ "() {}",
"  // call operator returns generator instances",
"  " +generatorName+ "* operator()(" +functorArgs+ ");",
"  // generator function body",
"  " +GeneratorFuncHeader("", nodeId)+ ";",
"};",
""
  };

  str += "\n";
  for (auto elem : genClsDecl)
    str += elem + "\n";
  return str;
}

std::string GeneratorClassDef(std::string ns, std::string funcName, unsigned nodeId) {
  std::string str;
  std::string generatorName = ns + GeneratorName(funcName);
  std::string generatorFuncName = ns + GeneratorFuncName(funcName);

  if (!ns.empty())
    funcName = ns + "::" + funcName;

  std::string params = FunctionParams(nodeId, false, false);
  std::string args   = FunctionParams(nodeId, false, true);
  if (!args.empty())
    args = ", " + args;

  str = R"""(
t2crt::IteratorResult* )""" + generatorName + R"""(::next(t2crt::JS_Val* arg) {
  if (_finished) {
    _res.done = true;
    return &_res;
  }
  // iterate by calling generation function with captures in generator
  )""" + funcName + R"""(->_body(this, _yield, _res)""" + args + R"""();
  if (_res.done == true)
    _finished = true;
  return &_res;
}

)""" + generatorName + "* "s + generatorFuncName + R"""(::operator()()""" + params + R"""() {
  return new )""" + generatorName + R"""((&t2crt::Generator, foo->prototype)""" + args + R"""();
}

)""";
  return str;
}

bool IsClassMethod(TreeNode* node) {
  return(node->IsFunction() && node->GetParent() && node->GetParent()->IsClass());
}

std::string tab(int n) {
  return std::string(n*2,' ');
}

std::string GenAnonFuncName(TreeNode* node) {
  return "_anon_func_"s + std::to_string(node->GetNodeId());
}

// return array constructor name of given type
// format:
//  1D array: t2crt::Array<type>::ctor
//  2D array: t2crt::Array<t2crt::Array<type>*>::ctor
//  3D array: t2crt::Array<t2crt::Array<t2crt::Array<type>*>*>::ctor
//  ...
//  note: must be in sycn with format generated by ARR_CTOR_DEF in builtins.h
std::string ArrayCtorName(int dim, std::string type) {
  if (!dim)
    return std::string();
  std::string str = "t2crt::Array<"s + type + ">"s;
  for (int i=1; i<dim; ++i) {
    str = "t2crt::Array<"s + str + "*>"s;
  }
  str = str + "::ctor"s;
  return str;
}

// note: entries below are to match values from ast nodes. Do not prepend with "t2crt::"
std::vector<std::string>builtins = {"Object", "Function", "Number", "Array", "Record"};

bool IsBuiltinObj(std::string name) {
  return std::find(builtins.begin(), builtins.end(), name) != builtins.end();
}

std::string ObjectTypeStr(std::string name) {
  if (IsBuiltinObj(name))
    return "t2crt::" + name + "*";
  else
    return name + "*";
}

} // namespace maplefe
