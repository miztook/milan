// Copyright (c) 2017 Valve Corporation
// Copyright (c) 2017 LunarG Inc.
//
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

#include "pass_fixture.h"
#include "pass_utils.h"

namespace {

using namespace spvtools;

using LocalSingleBlockLoadStoreElimTest = PassTest<::testing::Test>;

TEST_F(LocalSingleBlockLoadStoreElimTest, SimpleStoreLoadElim) {
  // #version 140
  //
  // in vec4 BaseColor;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     gl_FragColor = v;
  // }

  const std::string predefs_before =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%7 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %7
%13 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%14 = OpLoad %v4float %BaseColor
OpStore %v %14
%15 = OpLoad %v4float %v
OpStore %gl_FragColor %15
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %7
%13 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%14 = OpLoad %v4float %BaseColor
OpStore %v %14
%15 = OpLoad %v4float %v
OpStore %gl_FragColor %14
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<opt::LocalSingleBlockLoadStoreElimPass>(
      predefs_before + before, predefs_before + after, true, true);
}

TEST_F(LocalSingleBlockLoadStoreElimTest, SimpleLoadLoadElim) {
  // #version 140
  //
  // in vec4 BaseColor;
  // in float fi;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     if (fi < 0)
  //         v = vec4(0.0);
  //     gl_FragData[0] = v;
  //     gl_FragData[1] = v;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %fi %gl_FragData
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %fi "fi"
OpName %gl_FragData "gl_FragData"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%_ptr_Input_float = OpTypePointer Input %float
%fi = OpVariable %_ptr_Input_float Input
%float_0 = OpConstant %float 0
%bool = OpTypeBool
%16 = OpConstantComposite %v4float %float_0 %float_0 %float_0 %float_0
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%_arr_v4float_uint_32 = OpTypeArray %v4float %uint_32
%_ptr_Output__arr_v4float_uint_32 = OpTypePointer Output %_arr_v4float_uint_32
%gl_FragData = OpVariable %_ptr_Output__arr_v4float_uint_32 Output
%int = OpTypeInt 32 1
%int_0 = OpConstant %int 0
%_ptr_Output_v4float = OpTypePointer Output %v4float
%int_1 = OpConstant %int 1
)";

  const std::string before =
      R"(%main = OpFunction %void None %8
%25 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%26 = OpLoad %v4float %BaseColor
OpStore %v %26
%27 = OpLoad %float %fi
%28 = OpFOrdLessThan %bool %27 %float_0
OpSelectionMerge %29 None
OpBranchConditional %28 %30 %29
%30 = OpLabel
OpStore %v %16
OpBranch %29
%29 = OpLabel
%31 = OpLoad %v4float %v
%32 = OpAccessChain %_ptr_Output_v4float %gl_FragData %int_0
OpStore %32 %31
%33 = OpLoad %v4float %v
%34 = OpAccessChain %_ptr_Output_v4float %gl_FragData %int_1
OpStore %34 %33
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %8
%25 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%26 = OpLoad %v4float %BaseColor
OpStore %v %26
%27 = OpLoad %float %fi
%28 = OpFOrdLessThan %bool %27 %float_0
OpSelectionMerge %29 None
OpBranchConditional %28 %30 %29
%30 = OpLabel
OpStore %v %16
OpBranch %29
%29 = OpLabel
%31 = OpLoad %v4float %v
%32 = OpAccessChain %_ptr_Output_v4float %gl_FragData %int_0
OpStore %32 %31
%33 = OpLoad %v4float %v
%34 = OpAccessChain %_ptr_Output_v4float %gl_FragData %int_1
OpStore %34 %31
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<opt::LocalSingleBlockLoadStoreElimPass>(
      predefs + before, predefs + after, true, true);
}

TEST_F(LocalSingleBlockLoadStoreElimTest,
       NoStoreElimIfInterveningAccessChainLoad) {
  //
  // Note that even though the Load to %v is eliminated, the Store to %v
  // is not eliminated due to the following access chain reference.
  //
  // #version 140
  //
  // in vec4 BaseColor;
  // flat in int Idx;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     float f = v[Idx];
  //     gl_FragColor = v/f;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %Idx %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %f "f"
OpName %Idx "Idx"
OpName %gl_FragColor "gl_FragColor"
OpDecorate %Idx Flat
%void = OpTypeVoid
%9 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%_ptr_Function_float = OpTypePointer Function %float
%int = OpTypeInt 32 1
%_ptr_Input_int = OpTypePointer Input %int
%Idx = OpVariable %_ptr_Input_int Input
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
)";

  const std::string before =
      R"(%main = OpFunction %void None %9
%18 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%f = OpVariable %_ptr_Function_float Function
%19 = OpLoad %v4float %BaseColor
OpStore %v %19
%20 = OpLoad %int %Idx
%21 = OpAccessChain %_ptr_Function_float %v %20
%22 = OpLoad %float %21
OpStore %f %22
%23 = OpLoad %v4float %v
%24 = OpLoad %float %f
%25 = OpCompositeConstruct %v4float %24 %24 %24 %24
%26 = OpFDiv %v4float %23 %25
OpStore %gl_FragColor %26
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %9
%18 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%f = OpVariable %_ptr_Function_float Function
%19 = OpLoad %v4float %BaseColor
OpStore %v %19
%20 = OpLoad %int %Idx
%21 = OpAccessChain %_ptr_Function_float %v %20
%22 = OpLoad %float %21
OpStore %f %22
%23 = OpLoad %v4float %v
%24 = OpLoad %float %f
%25 = OpCompositeConstruct %v4float %22 %22 %22 %22
%26 = OpFDiv %v4float %19 %25
OpStore %gl_FragColor %26
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<opt::LocalSingleBlockLoadStoreElimPass>(
      predefs + before, predefs + after, true, true);
}

TEST_F(LocalSingleBlockLoadStoreElimTest, NoElimIfInterveningAccessChainStore) {
  // #version 140
  //
  // in vec4 BaseColor;
  // flat in int Idx;
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     v[Idx] = 0;
  //     gl_FragColor = v;
  // }

  const std::string assembly =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %Idx %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %Idx "Idx"
OpName %gl_FragColor "gl_FragColor"
OpDecorate %Idx Flat
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%int = OpTypeInt 32 1
%_ptr_Input_int = OpTypePointer Input %int
%Idx = OpVariable %_ptr_Input_int Input
%float_0 = OpConstant %float 0
%_ptr_Function_float = OpTypePointer Function %float
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
%main = OpFunction %void None %8
%18 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%19 = OpLoad %v4float %BaseColor
OpStore %v %19
%20 = OpLoad %int %Idx
%21 = OpAccessChain %_ptr_Function_float %v %20
OpStore %21 %float_0
%22 = OpLoad %v4float %v
OpStore %gl_FragColor %22
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<opt::LocalSingleBlockLoadStoreElimPass>(
      assembly, assembly, false, true);
}

TEST_F(LocalSingleBlockLoadStoreElimTest, NoElimIfInterveningFunctionCall) {
  // #version 140
  //
  // in vec4 BaseColor;
  //
  // void foo() {
  // }
  //
  // void main()
  // {
  //     vec4 v = BaseColor;
  //     foo();
  //     gl_FragColor = v;
  // }

  const std::string assembly =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %gl_FragColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %foo_ "foo("
OpName %v "v"
OpName %BaseColor "BaseColor"
OpName %gl_FragColor "gl_FragColor"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
%main = OpFunction %void None %8
%14 = OpLabel
%v = OpVariable %_ptr_Function_v4float Function
%15 = OpLoad %v4float %BaseColor
OpStore %v %15
%16 = OpFunctionCall %void %foo_
%17 = OpLoad %v4float %v
OpStore %gl_FragColor %17
OpReturn
OpFunctionEnd
%foo_ = OpFunction %void None %8
%18 = OpLabel
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<opt::LocalSingleBlockLoadStoreElimPass>(
      assembly, assembly, false, true);
}

TEST_F(LocalSingleBlockLoadStoreElimTest, ElimIfCopyObjectInFunction) {
  // Note: SPIR-V hand edited to insert CopyObject
  //
  // #version 140
  //
  // in vec4 BaseColor;
  //
  // void main()
  // {
  //   vec4 v1 = BaseColor;
  //   gl_FragData[0] = v1;
  //   vec4 v2 = BaseColor * 0.5;
  //   gl_FragData[1] = v2;
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %BaseColor %gl_FragData
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %v1 "v1"
OpName %BaseColor "BaseColor"
OpName %gl_FragData "gl_FragData"
OpName %v2 "v2"
%void = OpTypeVoid
%8 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%uint = OpTypeInt 32 0
%uint_32 = OpConstant %uint 32
%_arr_v4float_uint_32 = OpTypeArray %v4float %uint_32
%_ptr_Output__arr_v4float_uint_32 = OpTypePointer Output %_arr_v4float_uint_32
%gl_FragData = OpVariable %_ptr_Output__arr_v4float_uint_32 Output
%int = OpTypeInt 32 1
%int_0 = OpConstant %int 0
%_ptr_Output_v4float = OpTypePointer Output %v4float
%float_0_5 = OpConstant %float 0.5
%int_1 = OpConstant %int 1
)";

  const std::string before =
      R"(%main = OpFunction %void None %8
%22 = OpLabel
%v1 = OpVariable %_ptr_Function_v4float Function
%v2 = OpVariable %_ptr_Function_v4float Function
%23 = OpLoad %v4float %BaseColor
OpStore %v1 %23
%24 = OpLoad %v4float %v1
%25 = OpAccessChain %_ptr_Output_v4float %gl_FragData %int_0
OpStore %25 %24
%26 = OpLoad %v4float %BaseColor
%27 = OpVectorTimesScalar %v4float %26 %float_0_5
%28 = OpCopyObject %_ptr_Function_v4float %v2
OpStore %28 %27
%29 = OpLoad %v4float %28
%30 = OpAccessChain %_ptr_Output_v4float %gl_FragData %int_1
OpStore %30 %29
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %8
%22 = OpLabel
%v1 = OpVariable %_ptr_Function_v4float Function
%v2 = OpVariable %_ptr_Function_v4float Function
%23 = OpLoad %v4float %BaseColor
OpStore %v1 %23
%24 = OpLoad %v4float %v1
%25 = OpAccessChain %_ptr_Output_v4float %gl_FragData %int_0
OpStore %25 %23
%26 = OpLoad %v4float %BaseColor
%27 = OpVectorTimesScalar %v4float %26 %float_0_5
%28 = OpCopyObject %_ptr_Function_v4float %v2
OpStore %28 %27
%29 = OpLoad %v4float %28
%30 = OpAccessChain %_ptr_Output_v4float %gl_FragData %int_1
OpStore %30 %27
OpReturn
OpFunctionEnd
)";

  SinglePassRunAndCheck<opt::LocalSingleBlockLoadStoreElimPass>(
      predefs + before, predefs + after, true, true);
}

TEST_F(LocalSingleBlockLoadStoreElimTest, ElimOpaque) {
  // SPIR-V not representable in GLSL; not generatable from HLSL
  // at the moment

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %outColor %texCoords
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %S_t "S_t"
OpMemberName %S_t 0 "v0"
OpMemberName %S_t 1 "v1"
OpMemberName %S_t 2 "smp"
OpName %outColor "outColor"
OpName %sampler15 "sampler15"
OpName %s0 "s0"
OpName %texCoords "texCoords"
OpName %param "param"
OpDecorate %sampler15 DescriptorSet 0
%void = OpTypeVoid
%12 = OpTypeFunction %void
%float = OpTypeFloat 32
%v2float = OpTypeVector %float 2
%v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%outColor = OpVariable %_ptr_Output_v4float Output
%17 = OpTypeImage %float 2D 0 0 0 1 Unknown
%18 = OpTypeSampledImage %17
%S_t = OpTypeStruct %v2float %v2float %18
%_ptr_Function_S_t = OpTypePointer Function %S_t
%20 = OpTypeFunction %void %_ptr_Function_S_t
%_ptr_UniformConstant_18 = OpTypePointer UniformConstant %18
%_ptr_Function_18 = OpTypePointer Function %18
%sampler15 = OpVariable %_ptr_UniformConstant_18 UniformConstant
%int = OpTypeInt 32 1
%int_0 = OpConstant %int 0
%int_2 = OpConstant %int 2
%_ptr_Function_v2float = OpTypePointer Function %v2float
%_ptr_Input_v2float = OpTypePointer Input %v2float
%texCoords = OpVariable %_ptr_Input_v2float Input
)";

  const std::string before =
      R"(%main = OpFunction %void None %12
%28 = OpLabel
%s0 = OpVariable %_ptr_Function_S_t Function 
%param = OpVariable %_ptr_Function_S_t Function
%29 = OpLoad %v2float %texCoords
%30 = OpLoad %S_t %s0 
%31 = OpCompositeInsert %S_t %29 %30 0
OpStore %s0 %31
%32 = OpLoad %18 %sampler15
%33 = OpLoad %S_t %s0 
%34 = OpCompositeInsert %S_t %32 %33 2
OpStore %s0 %34
%35 = OpLoad %S_t %s0 
OpStore %param %35
%36 = OpLoad %S_t %param
%37 = OpCompositeExtract %18 %36 2
%38 = OpLoad %S_t %param
%39 = OpCompositeExtract %v2float %38 0
%40 = OpImageSampleImplicitLod %v4float %37 %39
OpStore %outColor %40
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(%main = OpFunction %void None %12
%28 = OpLabel
%s0 = OpVariable %_ptr_Function_S_t Function
%param = OpVariable %_ptr_Function_S_t Function
%29 = OpLoad %v2float %texCoords
%30 = OpLoad %S_t %s0
%31 = OpCompositeInsert %S_t %29 %30 0
OpStore %s0 %31
%32 = OpLoad %18 %sampler15
%33 = OpLoad %S_t %s0
%34 = OpCompositeInsert %S_t %32 %31 2
OpStore %s0 %34
%35 = OpLoad %S_t %s0
OpStore %param %34
%36 = OpLoad %S_t %param
%37 = OpCompositeExtract %18 %34 2
%38 = OpLoad %S_t %param
%39 = OpCompositeExtract %v2float %34 0
%40 = OpImageSampleImplicitLod %v4float %37 %39
OpStore %outColor %40
OpReturn
OpFunctionEnd
)";

  SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  SinglePassRunAndCheck<opt::LocalSingleBlockLoadStoreElimPass>(
      predefs + before, predefs + after, true, true);
}

TEST_F(LocalSingleBlockLoadStoreElimTest, PositiveAndNegativeCallTree) {
  // Note that the call tree function bar is optimized, but foo is not
  //
  // #version 140
  //
  // in vec4 BaseColor;
  //
  // vec4 foo(vec4 v1)
  // {
  //     vec4 t = v1;
  //     return t;
  // }
  //
  // vec4 bar(vec4 v1)
  // {
  //     vec4 t = v1;
  //     return t;
  // }
  //
  // void main()
  // {
  //     gl_FragColor = bar(BaseColor);
  // }

  const std::string predefs =
      R"(OpCapability Shader
%1 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %gl_FragColor %BaseColor
OpExecutionMode %main OriginUpperLeft
OpSource GLSL 140
OpName %main "main"
OpName %foo_vf4_ "foo(vf4;"
OpName %v1 "v1"
OpName %bar_vf4_ "bar(vf4;"
OpName %v1_0 "v1"
OpName %t "t"
OpName %t_0 "t"
OpName %gl_FragColor "gl_FragColor"
OpName %BaseColor "BaseColor"
OpName %param "param"
%void = OpTypeVoid
%13 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Function_v4float = OpTypePointer Function %v4float
%17 = OpTypeFunction %v4float %_ptr_Function_v4float
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_FragColor = OpVariable %_ptr_Output_v4float Output
%_ptr_Input_v4float = OpTypePointer Input %v4float
%BaseColor = OpVariable %_ptr_Input_v4float Input
%main = OpFunction %void None %13
%20 = OpLabel
%param = OpVariable %_ptr_Function_v4float Function
%21 = OpLoad %v4float %BaseColor
OpStore %param %21
%22 = OpFunctionCall %v4float %bar_vf4_ %param
OpStore %gl_FragColor %22
OpReturn
OpFunctionEnd
)";

  const std::string before =
      R"(%foo_vf4_ = OpFunction %v4float None %17
%v1 = OpFunctionParameter %_ptr_Function_v4float
%23 = OpLabel
%t = OpVariable %_ptr_Function_v4float Function
%24 = OpLoad %v4float %v1
OpStore %t %24
%25 = OpLoad %v4float %t
OpReturnValue %25
OpFunctionEnd
%bar_vf4_ = OpFunction %v4float None %17
%v1_0 = OpFunctionParameter %_ptr_Function_v4float
%26 = OpLabel
%t_0 = OpVariable %_ptr_Function_v4float Function
%27 = OpLoad %v4float %v1_0
OpStore %t_0 %27
%28 = OpLoad %v4float %t_0
OpReturnValue %28
OpFunctionEnd
)";

  const std::string after =
      R"(%foo_vf4_ = OpFunction %v4float None %17
%v1 = OpFunctionParameter %_ptr_Function_v4float
%23 = OpLabel
%t = OpVariable %_ptr_Function_v4float Function
%24 = OpLoad %v4float %v1
OpStore %t %24
%25 = OpLoad %v4float %t
OpReturnValue %25
OpFunctionEnd
%bar_vf4_ = OpFunction %v4float None %17
%v1_0 = OpFunctionParameter %_ptr_Function_v4float
%26 = OpLabel
%t_0 = OpVariable %_ptr_Function_v4float Function
%27 = OpLoad %v4float %v1_0
OpStore %t_0 %27
%28 = OpLoad %v4float %t_0
OpReturnValue %27
OpFunctionEnd
)";

  SinglePassRunAndCheck<opt::LocalSingleBlockLoadStoreElimPass>(
      predefs + before, predefs + after, true, true);
}

TEST_F(LocalSingleBlockLoadStoreElimTest, PointerVariable) {
  // Test that checks if a pointer variable is removed.

  const std::string before =
      R"(OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %1 "main" %2
OpExecutionMode %1 OriginUpperLeft
OpMemberDecorate %_struct_3 0 Offset 0
OpDecorate %_runtimearr__struct_3 ArrayStride 16
OpMemberDecorate %_struct_5 0 Offset 0
OpDecorate %_struct_5 BufferBlock
OpMemberDecorate %_struct_6 0 Offset 0
OpDecorate %_struct_6 BufferBlock
OpDecorate %2 Location 0
OpDecorate %7 DescriptorSet 0
OpDecorate %7 Binding 0
%void = OpTypeVoid
%10 = OpTypeFunction %void
%int = OpTypeInt 32 1
%uint = OpTypeInt 32 0
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%_ptr_Uniform_v4float = OpTypePointer Uniform %v4float
%_struct_3 = OpTypeStruct %v4float
%_runtimearr__struct_3 = OpTypeRuntimeArray %_struct_3
%_struct_5 = OpTypeStruct %_runtimearr__struct_3
%_ptr_Uniform__struct_5 = OpTypePointer Uniform %_struct_5
%_struct_6 = OpTypeStruct %int
%_ptr_Uniform__struct_6 = OpTypePointer Uniform %_struct_6
%_ptr_Function__ptr_Uniform__struct_5 = OpTypePointer Function %_ptr_Uniform__struct_5
%_ptr_Function__ptr_Uniform__struct_6 = OpTypePointer Function %_ptr_Uniform__struct_6
%int_0 = OpConstant %int 0
%uint_0 = OpConstant %uint 0
%2 = OpVariable %_ptr_Output_v4float Output
%7 = OpVariable %_ptr_Uniform__struct_5 Uniform
%1 = OpFunction %void None %10
%23 = OpLabel
%24 = OpVariable %_ptr_Function__ptr_Uniform__struct_5 Function
OpStore %24 %7
%26 = OpLoad %_ptr_Uniform__struct_5 %24
%27 = OpAccessChain %_ptr_Uniform_v4float %26 %int_0 %uint_0 %int_0
%28 = OpLoad %v4float %27
%29 = OpCopyObject %v4float %28
OpStore %2 %28
OpReturn
OpFunctionEnd
)";

  const std::string after =
      R"(OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %1 "main" %2
OpExecutionMode %1 OriginUpperLeft
OpMemberDecorate %_struct_3 0 Offset 0
OpDecorate %_runtimearr__struct_3 ArrayStride 16
OpMemberDecorate %_struct_5 0 Offset 0
OpDecorate %_struct_5 BufferBlock
OpMemberDecorate %_struct_6 0 Offset 0
OpDecorate %_struct_6 BufferBlock
OpDecorate %2 Location 0
OpDecorate %7 DescriptorSet 0
OpDecorate %7 Binding 0
%void = OpTypeVoid
%10 = OpTypeFunction %void
%int = OpTypeInt 32 1
%uint = OpTypeInt 32 0
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%_ptr_Uniform_v4float = OpTypePointer Uniform %v4float
%_struct_3 = OpTypeStruct %v4float
%_runtimearr__struct_3 = OpTypeRuntimeArray %_struct_3
%_struct_5 = OpTypeStruct %_runtimearr__struct_3
%_ptr_Uniform__struct_5 = OpTypePointer Uniform %_struct_5
%_struct_6 = OpTypeStruct %int
%_ptr_Uniform__struct_6 = OpTypePointer Uniform %_struct_6
%_ptr_Function__ptr_Uniform__struct_5 = OpTypePointer Function %_ptr_Uniform__struct_5
%_ptr_Function__ptr_Uniform__struct_6 = OpTypePointer Function %_ptr_Uniform__struct_6
%int_0 = OpConstant %int 0
%uint_0 = OpConstant %uint 0
%2 = OpVariable %_ptr_Output_v4float Output
%7 = OpVariable %_ptr_Uniform__struct_5 Uniform
%1 = OpFunction %void None %10
%23 = OpLabel
%24 = OpVariable %_ptr_Function__ptr_Uniform__struct_5 Function
OpStore %24 %7
%26 = OpLoad %_ptr_Uniform__struct_5 %24
%27 = OpAccessChain %_ptr_Uniform_v4float %7 %int_0 %uint_0 %int_0
%28 = OpLoad %v4float %27
%29 = OpCopyObject %v4float %28
OpStore %2 %28
OpReturn
OpFunctionEnd
)";

  SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  SinglePassRunAndCheck<opt::LocalSingleBlockLoadStoreElimPass>(before, after,
                                                                true, true);
}
// TODO(greg-lunarg): Add tests to verify handling of these cases:
//
//    Other target variable types
//    InBounds Access Chains
//    Check for correctness in the presence of function calls
//    Others?

}  // anonymous namespace
