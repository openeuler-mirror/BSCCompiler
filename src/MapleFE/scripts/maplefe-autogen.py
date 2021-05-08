#!/usr/bin/env python3
from os import path, environ
import subprocess
import ruamel.yaml as yaml

#
# Needs to install the following packages on Ubuntu 18.04 or 20.04
#   sudo apt install -y clang-tools-10 clang-format-10 python3-ruamel.yaml
#

root_dir = path.dirname(path.dirname(path.realpath(__file__))) + '/'
builddir = environ.get('BUILDDIR')
output_dir = builddir + '/ast_doc/' if builddir != None else root_dir + "output/typescript/ast_doc/"
maplefe_dir = root_dir + 'shared/'
# initial_yaml = output_dir + 'maplefe/index.yaml' # For higher version of clang-doc
initial_yaml = output_dir + 'maplefe.yaml'         # For version 10
treenode_yaml = output_dir + 'maplefe/TreeNode.yaml'

license_notice = [
        '/*',
        '* Copyright (C) [2021] Futurewei Technologies, Inc. All rights reverved.',
        '*',
        '* OpenArkFE is licensed under the Mulan PSL v2.',
        '* You can use this software according to the terms and conditions of the Mulan PSL v2.',
        '* You may obtain a copy of Mulan PSL v2 at:',
        '*',
        '*  http://license.coscl.org.cn/MulanPSL2',
        '*',
        '* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER',
        '* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR',
        '* FIT FOR A PARTICULAR PURPOSE.',
        '* See the Mulan PSL v2 for more details.',
        '*/',
        '',
        '// Generated by maplefe-autogen.py',
        '',
        ]

compile_commands = [
        '[',
        '  { "directory": "' + maplefe_dir + 'src"',
        '    "command":   "clang++ -std=c++17 -DDEBUG -fpermissive -I ' + maplefe_dir + 'include -w -c ast_builder.cpp",',
        '    "file":      "ast_builder.cpp",',
        '    "output":    "' + output_dir + '"',
        '  }',
        ']',
        ]

bash_commands = [
        'cd ' + maplefe_dir + 'src || exit 1',
        'rm -f ' + output_dir + 'yaml.log',
        'clang-doc-10 ast_builder.cpp -p ' + output_dir + ' --format=yaml -output=' + output_dir,
        ]

def exec_command(cmd):
    subprocess.call(cmd, shell=True)

def create(filename, lines):
    with open(filename, "w") as f:
        for line in lines:
            f.write(line + "\n")

def append(filename, lines):
    with open(filename, "a") as f:
        for line in lines:
            f.write(line + "\n")

def finalize(filename, lines):
    append(filename, lines)
    exec_command('clang-format-10 -i --style="{ColumnLimit: 120}" ' + filename)
    print("Generated " + filename)

exec_command('bash -c "mkdir -p ' + output_dir + 'shared"')
create(output_dir + 'compile_commands.json', compile_commands)
create(output_dir + 'ast.sh', bash_commands)
exec_command('bash ' + output_dir + 'ast.sh')

# Dump all content in a dictionary to ast_doc/yaml.log
def log(dictionary, indent, msg = ""):
    global log_buf
    if indent == 0: log_buf = [msg]
    indstr = " .  " * indent
    for key, value in dictionary.items():
        if key == "USR": continue
        prefix = indstr + key + ' : '
        if isinstance(value, dict):
            log_buf.append(prefix + "{")
            log(value, indent + 1)
            log_buf.append(indstr+ " }")
        elif isinstance(value, list):
            log_buf.append(prefix + "[")
            for elem in value:
                if isinstance(elem, dict):
                    log(elem, indent + 1)
                else:
                    log_buf.append(indstr + "   " + str(elem))
            log_buf.append(indstr+ " ]")
        else:
            log_buf.append(prefix + str(value))
    log_buf.append(indstr + "---")
    if indent == 0:
        append(output_dir + 'yaml.log', log_buf)

# Handle a YAML file with a callback
def handle_yaml(filename, callback, saved_yaml = {}):
    if filename not in saved_yaml:
        print(str(len(saved_yaml) + 1) + ": Processing " + filename + " ...")
        with open(filename) as stream:
            yaml_data = yaml.safe_load(stream)
        saved_yaml[filename] = yaml_data
        log(yaml_data, 0, "YAML file: " + filename)
    else:
        yaml_data = saved_yaml[filename]
    callback(yaml_data)

#################################################################

# Get the pointed-to type, e.g. FunctionNode of "class maplefe::FunctionNode *"
def get_pointed(mtype):
    loc = mtype.find("class maplefe::")
    return mtype[loc + 15:-2] if loc >= 0 and mtype[-6:] == "Node *" else None

# Get enum type, e.g. ImportProperty of "enum maplefe::ImportProperty"
def get_enum_type(mtype):
    loc = mtype.find("maplefe::")
    return mtype[loc + 9:] if loc >= 0 else None

# Get the enum list for given enum name
def get_enum_list(dictionary, enum_name):
    assert dictionary != None
    enums = dictionary["ChildEnums"]
    for e in enums:
        for key, value in e.items():
            if key == "Name" and value == enum_name:
                return e["Members"]
    return []

# Generate functions for enum types, e.g. "const char *GetEnumOprId(OprId k);" for enum OprId
def gen_enum_func(dictionary):
    global include_file, src_file, gen_args
    hcode = ['']
    xcode = ['']
    for each in dictionary["ChildEnums"]:
        name = each["Name"]
        hcode.append("static const char* GetEnum" + name + "(" + name + " k);")
        xcode.append("const char* " + gen_args[1] + "::GetEnum" + name + "(" + name + " k) {")
        xcode.append("switch(k) {")
        for e in get_enum_list(dictionary, name):
            xcode.append("case " + e + ":")
            xcode.append('return "' + e + '";')
        xcode.append("default: ;  // Unexpected kind")
        xcode.append("}")
        xcode.append('return "UNEXPECTED ' + name + '";')
        xcode.append("}\n")
    append(src_file, xcode)
    append(include_file, hcode)

# Generate code for class node which is derived from TreeNode
def gen_handler_ast_node(dictionary):
    global include_file, src_file, gen_args
    code = ['']
    node_name = dictionary["Name"];
    assert dictionary["TagType"] == "Class"

    member_functions = {}
    child_functions = dictionary.get("ChildFunctions")
    if child_functions != None:
        for c in child_functions:
            name = c.get("Name")
            member_functions[name] = "R-" + str(c.get("ReturnType").get("Type").get("Name"))

    # gen_func_definition() for the code at the beginning of current function body
    code.append(gen_func_definition(dictionary, node_name))
    members = dictionary.get("Members")
    if members != None:
        declloc = dictionary.get("DefLocation")
        if declloc != None and isinstance(declloc, dict):
            fname = declloc.get("Filename")
            floc = fname.find("shared/")
            code.append("// Declared at " + fname[floc:] + ":" + str(declloc.get("LineNumber")))

        for m in members:
            name = m.get("Name")
            assert name[0:1] == "m"
            otype = m.get("Type").get("Name")
            if otype == "_Bool": otype = "bool"

            if name == "mChildren":
                plural = "Get" + name[1:]
                singular = "GetChild"
            elif name[:3] == "mIs" and otype == "bool":
                plural = name[1:]
                singular = name[1:]
            else:
                plural = "Get" + name[1:]
                singular = "Get" + name[1:-1] if name[-7:] != "Classes" else "Get" + name[1:-2]

            ntype = get_pointed(otype)
            access = m.get("Access")
            accessstr = access if access != None else ""
            if ntype != None:
                prefix = "const_cast<" + ntype + "*>(" if otype[:6] == "const " else ''
                suffix = ")" if prefix != '' else ''
                if member_functions.get(plural) != None:
                    # gen_call_child_node() for child node in current function body
                    code.append(gen_call_child_node(dictionary, name, ntype, prefix + "node->" + plural + "()" + suffix))
                else:
                    # It is an ERROR if no member function for the child node
                    code.append("Error!; // " + gen_call_child_node(dictionary, name, ntype, prefix + "node->" + plural + "()" + suffix))
            elif ((otype == "SmallVector" or otype == "SmallList" or otype == "ExprListNode")
                    and member_functions.get(plural + "Num") != None
                    and (member_functions.get(singular) != None or member_functions.get(singular + "AtIndex") != None)):
                func_name = singular if member_functions.get(singular) != None else singular + "AtIndex"
                rtype = member_functions[func_name][2:]
                if rtype == "_Bool": rtype = "bool"
                ntype = get_pointed(rtype)
                if ntype != None or gen_call_handle_values():
                    # gen_call_children_node() for list or vector of nodes before entering the loop
                    code.append(gen_call_children_node(dictionary, name, otype + "<" + rtype + ">", "node->" + plural + "Num()"))
                    code.append("for(unsigned i = 0; i < node->" + plural + "Num(); ++i) {")
                    if ntype != None:
                        prefix = "const_cast<" + ntype + "*>(" if rtype[:6] == "const " else ''
                        suffix = ")" if prefix != '' else ''
                        # gen_call_nth_child_node() for the nth child node in the loop for the list or vector
                        code.append(gen_call_nth_child_node(dictionary, name, ntype, prefix + "node->" + func_name + "(i)" + suffix))
                    else:
                        # gen_call_nth_child_value() for the nth child value in the loop for the list or vector
                        code.append(gen_call_nth_child_value(dictionary, name, rtype, "node->" + func_name + "(i)"))
                    code.append("}")
                code.append(gen_call_children_node_end(dictionary, name, otype + "<" + rtype + ">", "node->" + plural + "Num()"))
            elif gen_call_handle_values():
                if member_functions.get(plural) != None:
                    # gen_call_child_value() for child value in current function body
                    code.append(gen_call_child_value(dictionary, name, otype, "node->" + plural + "()"))
                else:
                    # It is an ERROR if no member function for the child value
                    code.append("Error!; // " + gen_call_child_value(dictionary, name, otype, "node->" + plural + "()"))

    # gen_func_definition_end() for the code at the end of current function body
    code.append(gen_func_definition_end(dictionary, node_name))
    append(src_file, code)

    code = []
    code.append(gen_func_declaration(dictionary, node_name))
    append(include_file, code)

# Generate handler for TreeNode
def gen_handler_ast_TreeNode(dictionary):
    global include_file, src_file, gen_args
    code = ['']
    code.append(gen_func_declaration(dictionary, "TreeNode"))
    append(include_file, code)

    code = ['']
    code.append(gen_func_definition(dictionary, "TreeNode"))

    code.append("switch(node->GetKind()) {")
    for flag in get_enum_list(dictionary, "NodeKind"):
        code.append("case " + flag + ":");
        node_name = flag[3:] + "Node"
        filename = output_dir + 'maplefe/' + node_name + '.yaml'
        if path.exists(filename):
            # gen_call_child_node() for visiting child node
            code.append(gen_call_child_node(dictionary, "", node_name, "static_cast<" + node_name + "*>(node)"))
        elif node_name == "NullNode":
            code.append("// Ignore NullNode")
        else:
            # it is an ERROR if the node kind is out of range
            code.append("Error!!! // " + gen_call_child_node(dictionary, "", node_name, "static_cast<" + node_name + "*>(node)"))
        code.append("break;");
    code.append("default: ;  // Unexpected kind")
    code.append("}")
    code.append(gen_func_definition_end(dictionary, "TreeNode"))
    append(src_file, code)

# Handle each node which has TreeNode as its base
def gen_handler_ast_node_file(dictionary):
    base = dictionary.get("Bases")
    if base != None:
        basename = base[0].get("Name")
        if basename == "TreeNode":
            gen_handler_ast_node(dictionary)

# Check each child records
def gen_handler(dictionary):
    child_records = dictionary["ChildRecords"]
    for child in child_records:
        value = child["Name"]
        filename = output_dir + 'maplefe/' + value + '.yaml'
        if path.exists(filename):
            handle_yaml(filename, gen_handler_ast_node_file)
    # Generate handler for TreeNode
    gen_handler_ast_TreeNode(dictionary)

###################################################################################################

# Initialize/finalize include_file and src_file with gen_args
Initialization = 1
Finalization = 2
def handle_src_include_files(phase):
    global include_file, src_file, gen_args
    include_file = output_dir + "shared/" + gen_args[0] + ".h"
    src_file = output_dir + "shared/" + gen_args[0] + ".cpp"
    include_start = [
            '#ifndef __' + gen_args[1].upper() + '_HEADER__',
            '#define __' + gen_args[1].upper() + '_HEADER__',
            '',
            '#include "ast_module.h"',
            '#include "ast.h"',
            '#include "ast_type.h"',
            '#include "ast_attr.h"',
            '',
            'namespace maplefe {',
            '',
            'class ' + gen_args[1] + ' {',
            'public:',
            ]
    include_end = [
            '};',
            '',
            '}',
            '#endif',
            ]
    src_start = [
            '#include "' + gen_args[0] + '.h"',
            '',
            'namespace maplefe {',
            '',
            ]
    src_end = [
            '',
            '}',
            ]
    if phase == Initialization:
        create(include_file, license_notice + include_start)
        create(src_file, license_notice + src_start)
    elif phase == Finalization:
        finalize(include_file, include_end)
        finalize(src_file, src_end)

###################################################################################################

def get_data_based_on_type(val_type, accessor):
    e = get_enum_type(val_type)
    if e != None:
        return e + ': " + GetEnum' + e + '(' + accessor + '));'
    elif val_type == "LitData":
        return 'LitData: LitId, " + GetEnumLitId(' + accessor + '.mType) + ", " + GetEnumLitData(' + accessor + '));'
    elif val_type == "bool":
        return val_type + ', ", ' + accessor + ');'
    elif val_type == 'unsigned int' or val_type == 'uint32_t' or val_type == 'uint64_t' \
            or val_type == 'unsigned' or val_type == 'int' or val_type == 'int32_t' or val_type == 'int64_t' :
        return val_type + ', " + std::to_string(' + accessor + '));'
    elif val_type == 'const char *':
        return 'const char*, " + (' + accessor + ' ? std::string("\\"") + ' + accessor + ' + "\\"" : "null"));'
    return val_type + ', " + "value"); // Warning: failed to get value'

def short_name(node_type):
    return node_type.replace('class ', '').replace('maplefe::', '').replace(' *', '*')

def padding_name(name):
    return gen_args[3] + name.ljust(7)

# The follwoing gen_func_* and gen_call* functions are for AstDump
gen_call_handle_values = lambda: True
gen_func_declaration = lambda dictionary, node_name: \
        "void " + gen_args[2] + node_name + "(" + node_name + "* node);"
gen_func_definition = lambda dictionary, node_name: \
        "void " + gen_args[1] + "::" + gen_args[2] + node_name + "(" + node_name + "* node) {" \
        + ('if (node == nullptr) \nreturn;\n' if node_name == "TreeNode" else '\nif(DumpFB("' + node_name \
        + '", node)) { MASSERT(node->GetKind() == NK_' + node_name.replace('Node', '') + ');')
gen_call_child_node = lambda dictionary, field_name, node_type, accessor: \
        ('Dump("' + padding_name(field_name) + ': ' + short_name(node_type) + '*", ' + accessor  + ');\n' \
        if field_name != '' else '') + gen_args[2] + short_name(node_type) + '(' + accessor + ');'
gen_call_child_value = lambda dictionary, field_name, val_type, accessor: \
        'Dump(std::string("' + padding_name(field_name) + ': ") + "' + get_data_based_on_type(val_type, accessor)
gen_call_children_node = lambda dictionary, field_name, node_type, accessor: \
        'DumpLB("' + padding_name(field_name) + ': ' + short_name(node_type) + ', size=", ' + accessor+ ');'
gen_call_children_node_end = lambda dictionary, field_name, node_type, accessor: 'DumpLE(' + accessor + ');'
gen_call_nth_child_node = lambda dictionary, field_name, node_type, accessor: \
        'Dump(std::to_string(i + 1) + ": ' + short_name(node_type) + '*", ' + accessor + ');\n' \
        + gen_args[2] + short_name(node_type) + '(' + accessor + ');'
gen_call_nth_child_value = lambda dictionary, field_name, val_type, accessor: \
        'Dump(std::to_string(i) + ". ' + get_data_based_on_type(val_type, accessor)
gen_func_definition_end = lambda dictionary, node_name: \
        'return;\n}' if node_name == "TreeNode" else 'DumpFE();\n}\nreturn;\n}'

#
# Generate source files for dumping AST
#
gen_args = [
        "gen_astdump", # filename
        "AstDump",     # Class name
        "AstDump",     # Prefix of function name
        "",            # Prefix of generated string literal for field name
        ]

astdump_init = [
        'private:',
        'int indent;',
        'std::string indstr;',
        '',
        'public:',
        gen_args[1] + '() : indent(0) {'
        'indstr = std::string(256, \' \');',
        'for(int i = 2; i < 256; i += 4)',
        'indstr.at(i) = \'.\';',
        '}', '',
        'void Dump(TreeNode* node) {',
        'AstDumpTreeNode(node);',
        '}', '',
        'private:',
        'void Dump(const std::string& msg) {',
        'std::cout << indstr.substr(0, indent) << msg << std::endl;',
        '}', '',
        'void Dump(const std::string& msg, TreeNode *node) {',
        'std::cout << indstr.substr(0, indent) << msg << (node ? "" : ", null") << std::endl;',
        '}', '',
        'void Dump(const std::string& msg, bool val) {',
        'std::cout << indstr.substr(0, indent) << msg << (val ? "true" : "false") << std::endl;',
        '}', '',
        'TreeNode* DumpFB(const std::string& msg, TreeNode* node) {',
        'if (node != nullptr) {',
        'std::cout << indstr.substr(0, indent + 2) << msg;',
        'indent += 4;', 'std::cout << " {" << std::endl;', 'DumpTreeNode(node);',
        '}', 'return node;',
        '}', '',
        'void DumpFE() {',
        'indent -= 4;', 'std::cout << indstr.substr(0, indent + 2) << "}" << std::endl;',
        '}', '',
        'void DumpLB(const std::string& msg, unsigned size) {',
        'std::cout << indstr.substr(0, indent) << msg << size << (size ? " [" : "") << std::endl;', 'indent += 4;',
        '}', '',
        'void DumpLE(unsigned size) {',
        'indent -= 4;',
        'if(size)', 'std::cout << indstr.substr(0, indent + 2) << "]" << std::endl;',
        '}', '',
        'std::string GetEnumLitData(LitData lit) {',
        'std::string str = GetEnumLitId(lit.mType);',
        'switch (lit.mType) {',
        'case LT_IntegerLiteral:',
        'return std::to_string(lit.mData.mInt);',
        'case LT_FPLiteral:',
        'return std::to_string(lit.mData.mFloat);',
        'case LT_DoubleLiteral:',
        'return std::to_string(lit.mData.mDouble);',
        'case LT_BooleanLiteral:',
        'return std::to_string(lit.mData.mBool);',
        'case LT_CharacterLiteral:',
        'return std::string(1, lit.mData.mChar.mData.mChar); // TODO: Unicode support',
        'case LT_StringLiteral:',
        'return std::string(lit.mData.mStr);',
        'case LT_NullLiteral:',
        'return std::string("null");',
        'case LT_ThisLiteral:',
        'return std::string("this");',
        'case LT_SuperLiteral:',
        'return std::string("super");',
        'case LT_NA:',
        'return std::string("NA");',
        'default:;',
        '}',
        '}', '',
        ]

handle_src_include_files(Initialization)
append(include_file, astdump_init)
handle_yaml(initial_yaml, gen_handler)
append(include_file, ['','public:'])
handle_yaml(initial_yaml, gen_enum_func)
gen_args[2] = "Dump"
gen_args[3] = "^ "
gen_call_child_node = lambda dictionary, field_name, node_type, accessor: \
    ('Dump("' + padding_name(field_name) + ': ' + short_name(node_type) \
    + '*, " + (' + accessor + ' ? "NodeId=" + std::to_string(' + accessor \
    + '->GetNodeId()) : std::string("null")));\n' if field_name != '' else '')
handle_yaml(treenode_yaml, gen_handler_ast_node)
handle_src_include_files(Finalization)

################################################################################

# The follwoing gen_func_* and gen_call* functions are for AstVisitor
gen_call_handle_values = lambda: False
gen_func_declaration = lambda dictionary, node_name: \
        'virtual ' + node_name + '* ' + gen_args[2] + node_name + '(' + node_name + '* node);'
gen_func_definition = lambda dictionary, node_name: \
        node_name + '* ' + gen_args[1] + '::' + gen_args[2] + node_name + '(' + node_name + '* node) {\nif(node != nullptr) {' \
        + ('\nif(mTrace) std::cout << "Visiting ' + node_name + ', id=" << node->GetNodeId() << "..." << std::endl;' \
        if node_name != 'TreeNode' else '')
gen_call_child_node = lambda dictionary, field_name, node_type, accessor: \
        'if(auto t = ' + accessor + ') ' + gen_args[2] + node_type + '(t);'
gen_call_children_node = lambda dictionary, field_name, node_type, accessor: ''
gen_call_children_node_end = lambda dictionary, field_name, node_type, accessor: ''
gen_call_nth_child_node = lambda dictionary, field_name, node_type, accessor: \
        'if(auto t = ' + accessor + ') ' + gen_args[2] + node_type + '(t);'
gen_func_definition_end = lambda dictionary, node_name: '}\nreturn node;\n}'

#
# Generate gen_handler.h and gen_handler.cpp
#
gen_args = [
        "gen_astvisitor", # filename
        "AstVisitor",     # Class name
        "Visit",          # Prefix of function name
        ]

astvisitor_init = [
        'private:',
        'bool mTrace;', '',
        'public:',
        gen_args[1] + '(bool t = false) : mTrace(t) {}', '',
        'TreeNode* ' + gen_args[2] + '(TreeNode* node) {',
        'return ' + gen_args[2] + 'TreeNode(node);',
        '}', '',
        ]

# Example to extract code pieces starting from initial_yaml
handle_src_include_files(Initialization)
append(include_file, astvisitor_init)
handle_yaml(initial_yaml, gen_handler)
handle_src_include_files(Finalization)
