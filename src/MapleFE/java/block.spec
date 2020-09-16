# Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
#
# OpenArkFE is licensed under the Mulan PSL v1.
# You can use this software according to the terms and conditions of the Mulan PSL v1.
# You may obtain a copy of Mulan PSL v1 at:
#
#  http://license.coscl.org.cn/MulanPSL
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v1 for more details.
#
###################################################################################
# This file defines the Java Block/Class/Interface statement.
###################################################################################

rule ClassDeclaration : ONEOF(NormalClassDeclaration, EnumDeclaration)
rule NormalClassDeclaration : ZEROORMORE(ClassModifier) + "class" + Identifier +
                              ZEROORONE(TypeParameters) + ZEROORONE(Superclass) +
                              ZEROORONE(Superinterfaces) + ClassBody
attr.action : BuildClass(%3)
attr.action : AddModifier(%1)
attr.action : AddClassBody(%7)
attr.action : AddSuperClass(%5)
attr.action : AddSuperInterface(%6)

rule ClassAttr : ONEOF("public", "protected", "private", "abstract", "static", "final", "strictfp")
  attr.property : Single
rule ClassModifier : ONEOF(Annotation, ClassAttr)

# 1. Generic class
# 2. TypeParameter will be defined in type.spec
rule TypeParameters    : '<' + TypeParameterList + '>'
rule TypeParameterList : TypeParameter + ZEROORMORE(',' + TypeParameter)
rule TypeParameter     : ZEROORMORE(TypeParameterModifier) + Identifier + ZEROORONE(TypeBound)
rule TypeParameterModifier : Annotation
rule TypeBound : ONEOF("extends" + TypeVariable,
                       "extends" + ClassOrInterfaceType + ZEROORMORE(AdditionalBound))
rule AdditionalBound : '&' + InterfaceType

# ClassType and InterfaceType are defined in type.spec
rule Superclass        : "extends" + ClassType
rule Superinterfaces   : "implements" + InterfaceTypeList
rule InterfaceTypeList : InterfaceType + ZEROORMORE(',' + InterfaceType)

# class body
rule ClassBody              : "{" + ZEROORMORE(ClassBodyDeclaration) + "}"
  attr.action: BuildBlock(%2)

rule ClassBodyDeclaration   : ONEOF(ClassMemberDeclaration,
                                    InstanceInitializer,
                                    StaticInitializer,
                                    ConstructorDeclaration)
  attr.property : Single

rule InstanceInitializer    : Block
  attr.action: BuildInstInit(%1)
rule StaticInitializer      : "static" + Block
  attr.action: BuildInstInit(%2)
  attr.action: AddModifierTo(%2, %1)

rule ClassMemberDeclaration : ONEOF(FieldDeclaration,
                                    MethodDeclaration,
                                    ClassDeclaration,
                                    InterfaceDeclaration,
                                    ';')
  attr.property : Single

rule FieldDeclaration  : ZEROORMORE(FieldModifier) + UnannType + VariableDeclaratorList + ';'
  attr.action: BuildDecl(%2, %3)
  attr.action: AddModifier(%1)

rule MethodDeclaration : ZEROORMORE(MethodModifier) + MethodHeader + MethodBody
  attr.action: AddModifierTo(%2, %1)
  attr.action: AddFunctionBodyTo(%2, %3)

rule MethodBody        : ONEOF(Block, ';')
  attr.property : Single
rule MethodHeader      : ONEOF(Result + MethodDeclarator + ZEROORONE(Throws),
                               TypeParameters + ZEROORMORE(Annotation) + Result + MethodDeclarator +
                               ZEROORONE(Throws))
  attr.action.%1: AddTypeTo(%2, %1)
  attr.action.%1: AddThrowsTo(%2, %3)
  attr.action.%2: AddThrowsTo(%4, %5)
  attr.property : Single

rule Result            : ONEOF(UnannType, "void")
  attr.property : Single
rule MethodDeclarator  : Identifier + '(' + ZEROORONE(FormalParameterList) + ')' + ZEROORONE(Dims)
  attr.action: BuildFunction(%1)
  attr.action: AddParams(%3)
  attr.action: AddDims(%5)

rule Throws            : "throws" + ExceptionTypeList
  attr.action: BuildThrows(%2)

rule ExceptionTypeList : ExceptionType + ZEROORMORE(',' + ExceptionType)
rule ExceptionType     : ONEOF(ClassType, TypeVariable)

rule MethodAttr : ONEOF("public", "protected", "private", "abstract", "static",
                        "final", "synchronized", "native", "strictfp")
  attr.property : Single

rule MethodModifier    : ONEOF(Annotation, MethodAttr)
  attr.property : Single

rule FormalParameterList : ONEOF(ReceiverParameter,
                                 FormalParameters + ',' + LastFormalParameter,
                                 LastFormalParameter)
  attr.action.%2: BuildVarList(%1, %3)
rule FormalParameters  : ONEOF(FormalParameter + ZEROORMORE(',' + FormalParameter),
                               ReceiverParameter + ZEROORMORE(',' + FormalParameter))
  attr.action.%1: BuildVarList(%1, %2)

rule FormalParameter   : ZEROORMORE(VariableModifier) + UnannType + VariableDeclaratorId
  attr.action: BuildDecl(%2, %3)
  attr.action: AddModifier(%1)
rule ReceiverParameter : ZEROORMORE(Annotation) + UnannType + ZEROORONE(Identifier + '.') + "this"
rule LastFormalParameter : ONEOF(ZEROORMORE(VariableModifier) + UnannType + ZEROORMORE(Annotation) +
                                   "..." + VariableDeclaratorId,
                                 FormalParameter)


rule FieldAttr : ONEOF("public", "protected", "private", "static", "final", "transient", "volatile")
  attr.property : Single
rule FieldModifier : ONEOF(Annotation, FieldAttr)
  attr.property : Single

################################################################
#                        Constructor                           #
################################################################
rule ConstructorDeclaration : ZEROORMORE(ConstructorModifier) + ConstructorDeclarator +
                              ZEROORONE(Throws) + ConstructorBody
  attr.action : AddFunctionBodyTo(%2, %4)

rule ConstructorAttr : ONEOF("public", "protected", "private")
rule ConstructorModifier    : ONEOF(Annotation, ConstructorAttr)
rule ConstructorDeclarator  : ZEROORONE(TypeParameters) + SimpleTypeName + '(' +
                              ZEROORONE(FormalParameterList) + ')'
  attr.action : BuildConstructor(%2)
rule SimpleTypeName         : Identifier
rule ConstructorBody        : '{' + ZEROORONE(ExplicitConstructorInvocation) +
                              ZEROORONE(BlockStatements) + '}'
  attr.action : BuildBlock(%3)

rule ExplicitConstructorInvocation : ONEOF(
         ZEROORONE(TypeArguments) + "this" + '(' + ZEROORONE(ArgumentList) + ')' +  ';',
         ZEROORONE(TypeArguments) + "super" + '(' + ZEROORONE(ArgumentList) + ')' +  ';',
         ExpressionName + '.' + ZEROORONE(TypeArguments) + "super" + '(' + ZEROORONE(ArgumentList) + ')' +  ';',
         Primary + '.' + ZEROORONE(TypeArguments) + "super" + '(' + ZEROORONE(ArgumentList) + ')' +  ';')

################
# A fake ones
rule EnumDeclaration        : "fakeenumdeclaration"

######################################################################
#                        Block                                       #
######################################################################
rule BlockStatement  : ONEOF(LocalVariableDeclarationStatement,
                             ClassDeclaration,
                             Statement)
rule BlockStatements : BlockStatement + ZEROORMORE(BlockStatement)
rule Block           : '{' + ZEROORONE(BlockStatements) + '}'
  attr.action: BuildBlock(%2)


######################################################################
#                        Interface                                   #
######################################################################
rule InterfaceDeclaration : ONEOF(NormalInterfaceDeclaration, AnnotationTypeDeclaration)
rule NormalInterfaceDeclaration : ZEROORMORE(InterfaceModifier) + "interface" + Identifier +
                                  ZEROORONE(TypeParameters) + ZEROORONE(ExtendsInterfaces) + InterfaceBody
rule InterfaceAttr : ONEOF("public", "protected", "private", "abstract", "static", "strictfp")
rule InterfaceModifier : ONEOF(Annotation, InterfaceAttr)
rule ExtendsInterfaces : "extends" + InterfaceTypeList
rule InterfaceBody     : '{' + ZEROORMORE(InterfaceMemberDeclaration) + '}'
rule InterfaceMemberDeclaration : ONEOF(ConstantDeclaration,
                                        InterfaceMethodDeclaration,
                                        ClassDeclaration,
                                        InterfaceDeclaration,
                                        ';')
# constant decl is also called field decl. In interface, field must have a variable initializer
# However, the rules below don't tell this limitation.
rule ConstantDeclaration : ZEROORMORE(ConstantModifier) + UnannType + VariableDeclaratorList + ';'
rule ConstantAttr : ONEOF("public", "static", "final")
rule ConstantModifier : ONEOF(Annotation, ConstantAttr)

rule InterfaceMethodDeclaration : ZEROORMORE(InterfaceMethodModifier) + MethodHeader + MethodBody
rule InterfaceMethodAttr : ONEOF("public", "abstract", "default", "static", "strictfp")
rule InterfaceMethodModifier : ONEOF(Annotation, InterfaceMethodAttr)

######################################################################
#                        Annotation Type                             #
######################################################################
rule AnnotationTypeDeclaration : ZEROORMORE(InterfaceModifier) + '@' + "interface" +
                                 Identifier + AnnotationTypeBody
  attr.action : BuildAnnotationType(%4)
  attr.action : AddModifier(%1)
  attr.action : AddAnnotationTypeBody(%5)

rule AnnotationTypeBody : '{' + ZEROORMORE(AnnotationTypeMemberDeclaration) + '}'
  attr.action : BuildBlock(%2)
rule AnnotationTypeMemberDeclaration : ONEOF(AnnotationTypeElementDeclaration,
                                             ConstantDeclaration,
                                             ClassDeclaration,
                                             InterfaceDeclaration,
                                             ';')
rule AnnotationTypeElementDeclaration : ZEROORMORE(AnnotationTypeElementModifier) + UnannType +
                                          Identifier + '(' +  ')' + ZEROORONE(Dims) +
                                          ZEROORONE(DefaultValue) + ';'
rule AnnotationTypeElementAttr : ONEOF("public", "abstract")
rule AnnotationTypeElementModifier : ONEOF(Annotation, AnnotationTypeElementAttr)
rule DefaultValue : "default" + ElementValue

######################################################################
#                        Annotation                                  #
######################################################################
rule Annotation : ONEOF(NormalAnnotation,
                        MarkerAnnotation,
                        SingleElementAnnotation)

rule NormalAnnotation : '@' + TypeName + '(' + ZEROORONE(ElementValuePairList) + ')'
  attr.action : BuildAnnotation(%2)

rule MarkerAnnotation : '@' + TypeName
  attr.action : BuildAnnotation(%2)

rule SingleElementAnnotation : '@' + TypeName + '(' + ElementValue + ')'
  attr.action : BuildAnnotation(%2)

rule ElementValuePairList : ElementValuePair + ZEROORMORE(',' + ElementValuePair)
rule ElementValuePair : Identifier + '=' + ElementValue
rule ElementValue : ONEOF(ConditionalExpression,
                          ElementValueArrayInitializer,
                          Annotation)
rule ElementValueArrayInitializer : '{' + ZEROORONE(ElementValueList) + ZEROORONE(',') + '}'
rule ElementValueList : ElementValue + ZEROORMORE(',' + ElementValue)

######################################################################
#                        Package                                     #
######################################################################
rule PackageModifier: Annotation
rule PackageDeclaration: ZEROORMORE(PackageModifier) + "package" + Identifier + ZEROORMORE('.' + Identifier) + ';'
  attr.action : BuildField(%3, %4)
  attr.action : BuildPackageName()

rule ImportDeclaration: ONEOF(SingleTypeImportDeclaration,
                              TypeImportOnDemandDeclaration,
                              SingleStaticImportDeclaration,
                              StaticImportOnDemandDeclaration)
rule SingleTypeImportDeclaration: "import" + TypeName + ';'
  attr.action : BuildSingleTypeImport(%2)
rule TypeImportOnDemandDeclaration: "import" + PackageOrTypeName + '.' + '*' + ';'
  attr.action : BuildAllTypeImport(%2)
rule SingleStaticImportDeclaration: "import" + "static" + TypeName + '.' + Identifier + ';'
  attr.action : BuildSingleStaticImport(%3, %4)
rule StaticImportOnDemandDeclaration: "import" + "static" + TypeName + '.' + '*' + ';'
  attr.action : BuildAllStaticImport(%3)

rule TypeDeclaration: ONEOF(ClassDeclaration,
                            InterfaceDeclaration,
                            ';')
