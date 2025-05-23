//===- SPIRVtype.cpp - Class to represent a SPIR-V type ---------*- C++ -*-===//
//
//                     The LLVM/SPIRV Translator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Copyright (c) 2014 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal with the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimers.
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimers in the documentation
// and/or other materials provided with the distribution.
// Neither the names of Advanced Micro Devices, Inc., nor the names of its
// contributors may be used to endorse or promote products derived from this
// Software without specific prior written permission.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
// THE SOFTWARE.
//
//===----------------------------------------------------------------------===//
/// \file
///
/// This file implements the types defined in SPIRV spec with op codes.
///
//===----------------------------------------------------------------------===//

#include "SPIRVType.h"
#include "SPIRVDecorate.h"
#include "SPIRVModule.h"
#include "SPIRVValue.h"

#include <cassert>

namespace SPIRV {

SPIRVType *SPIRVType::getArrayElementType() const {
  assert(OpCode == OpTypeArray && "Not array type");
  return static_cast<const SPIRVTypeArray *>(this)->getElementType();
}

uint64_t SPIRVType::getArrayLength() const {
  assert(OpCode == OpTypeArray && "Not array type");
  const SPIRVTypeArray *AsArray = static_cast<const SPIRVTypeArray *>(this);
  assert(AsArray->getLength()->getOpCode() == OpConstant &&
         "getArrayLength can only be called with constant array lengths");
  return static_cast<SPIRVConstant *>(AsArray->getLength())->getZExtIntValue();
}

SPIRVWord SPIRVType::getBitWidth() const {
  if (isTypeVector())
    return getVectorComponentType()->getBitWidth();
  if (isTypeBool())
    return 1;
  return isTypeInt() ? getIntegerBitWidth() : getFloatBitWidth();
}

SPIRVWord SPIRVType::getFloatBitWidth() const {
  assert(OpCode == OpTypeFloat && "Not a float type");
  return static_cast<const SPIRVTypeFloat *>(this)->getBitWidth();
}

SPIRVWord SPIRVType::getIntegerBitWidth() const {
  assert((OpCode == OpTypeInt || OpCode == OpTypeBool) &&
         "Not an integer type");
  if (isTypeBool())
    return 1;
  return static_cast<const SPIRVTypeInt *>(this)->getBitWidth();
}

SPIRVType *SPIRVType::getFunctionReturnType() const {
  assert(OpCode == OpTypeFunction);
  return static_cast<const SPIRVTypeFunction *>(this)->getReturnType();
}

SPIRVType *SPIRVType::getPointerElementType() const {
  assert((OpCode == OpTypePointer || OpCode == OpTypeUntypedPointerKHR) &&
         "Not a pointer type");
  if (OpCode == OpTypeUntypedPointerKHR)
    return const_cast<SPIRVType *>(this);
  return static_cast<const SPIRVTypePointer *>(this)->getElementType();
}

SPIRVStorageClassKind SPIRVType::getPointerStorageClass() const {
  assert((OpCode == OpTypePointer || OpCode == OpTypeUntypedPointerKHR) &&
         "Not a pointer type");
  return static_cast<const SPIRVTypePointer *>(this)->getStorageClass();
}

SPIRVType *SPIRVType::getStructMemberType(size_t Index) const {
  assert(OpCode == OpTypeStruct && "Not struct type");
  return static_cast<const SPIRVTypeStruct *>(this)->getMemberType(Index);
}

SPIRVWord SPIRVType::getStructMemberCount() const {
  assert(OpCode == OpTypeStruct && "Not struct type");
  return static_cast<const SPIRVTypeStruct *>(this)->getMemberCount();
}

SPIRVWord SPIRVType::getVectorComponentCount() const {
  assert(OpCode == OpTypeVector && "Not vector type");
  return static_cast<const SPIRVTypeVector *>(this)->getComponentCount();
}

SPIRVType *SPIRVType::getVectorComponentType() const {
  if (OpCode == OpTypeVector)
    return static_cast<const SPIRVTypeVector *>(this)->getComponentType();
  if (OpCode == internal::OpTypeJointMatrixINTEL)
    return static_cast<const SPIRVTypeJointMatrixINTEL *>(this)->getCompType();
  if (OpCode == OpTypeCooperativeMatrixKHR)
    return static_cast<const SPIRVTypeCooperativeMatrixKHR *>(this)
        ->getCompType();
  assert(0 && "getVectorComponentType(): Not a vector or joint matrix type");
  return nullptr;
}

SPIRVWord SPIRVType::getMatrixColumnCount() const {
  assert(OpCode == OpTypeMatrix && "Not matrix type");
  return static_cast<const SPIRVTypeMatrix *>(this)->getColumnCount();
}

SPIRVType *SPIRVType::getMatrixColumnType() const {
  assert(OpCode == OpTypeMatrix && "Not matrix type");
  return static_cast<const SPIRVTypeMatrix *>(this)->getColumnType();
}

SPIRVType *SPIRVType::getScalarType() const {
  switch (OpCode) {
  case OpTypePointer:
    return getPointerElementType()->getScalarType();
  case OpTypeArray:
    return getArrayElementType();
  case OpTypeVector:
    return getVectorComponentType();
  case OpTypeMatrix:
    return getMatrixColumnType()->getVectorComponentType();
  case OpTypeInt:
  case OpTypeFloat:
  case OpTypeBool:
    return const_cast<SPIRVType *>(this);
  default:
    break;
  }
  return nullptr;
}

bool SPIRVType::isTypeVoid() const { return OpCode == OpTypeVoid; }
bool SPIRVType::isTypeArray() const { return OpCode == OpTypeArray; }

bool SPIRVType::isTypeBool() const { return OpCode == OpTypeBool; }

bool SPIRVType::isTypeComposite() const {
  return isTypeVector() || isTypeArray() || isTypeStruct() ||
         isTypeJointMatrixINTEL() || isTypeCooperativeMatrixKHR();
}

bool SPIRVType::isTypeFloat(unsigned Bits,
                            unsigned FloatingPointEncoding) const {
  if (!isType<SPIRVTypeFloat>(this))
    return false;
  if (Bits == 0)
    return true;
  const auto *ThisFloat = static_cast<const SPIRVTypeFloat *>(this);
  return ThisFloat->getBitWidth() == Bits &&
         ThisFloat->getFloatingPointEncoding() == FloatingPointEncoding;
}

bool SPIRVType::isTypeOCLImage() const {
  return isTypeImage() &&
         static_cast<const SPIRVTypeImage *>(this)->isOCLImage();
}

bool SPIRVType::isTypePipe() const { return OpCode == OpTypePipe; }

bool SPIRVType::isTypePipeStorage() const {
  return OpCode == OpTypePipeStorage;
}

bool SPIRVType::isTypeReserveId() const { return OpCode == OpTypeReserveId; }

bool SPIRVType::isTypeInt(unsigned Bits) const {
  return isType<SPIRVTypeInt>(this, Bits);
}

bool SPIRVType::isTypePointer() const {
  return OpCode == OpTypePointer || OpCode == OpTypeUntypedPointerKHR;
}

bool SPIRVType::isTypeUntypedPointerKHR() const {
  return OpCode == OpTypeUntypedPointerKHR;
}

bool SPIRVType::isTypeOpaque() const { return OpCode == OpTypeOpaque; }

bool SPIRVType::isTypeEvent() const { return OpCode == OpTypeEvent; }

bool SPIRVType::isTypeDeviceEvent() const {
  return OpCode == OpTypeDeviceEvent;
}

bool SPIRVType::isTypeSampler() const { return OpCode == OpTypeSampler; }

bool SPIRVType::isTypeImage() const { return OpCode == OpTypeImage; }

bool SPIRVType::isTypeSampledImage() const {
  return OpCode == OpTypeSampledImage;
}

bool SPIRVType::isTypeStruct() const { return OpCode == OpTypeStruct; }

bool SPIRVType::isTypeVector() const { return OpCode == OpTypeVector; }

bool SPIRVType::isTypeJointMatrixINTEL() const {
  return OpCode == internal::OpTypeJointMatrixINTEL ||
         OpCode == internal::OpTypeJointMatrixINTELv2;
}

bool SPIRVType::isTypeCooperativeMatrixKHR() const {
  return OpCode == OpTypeCooperativeMatrixKHR;
}

bool SPIRVType::isTypeVectorBool() const {
  return isTypeVector() && getVectorComponentType()->isTypeBool();
}

bool SPIRVType::isTypeVectorInt() const {
  return isTypeVector() && getVectorComponentType()->isTypeInt();
}

bool SPIRVType::isTypeVectorFloat() const {
  return isTypeVector() && getVectorComponentType()->isTypeFloat();
}

bool SPIRVType::isTypeVectorOrScalarBool() const {
  return isTypeBool() || isTypeVectorBool();
}

bool SPIRVType::isTypeVectorPointer() const {
  return isTypeVector() && getVectorComponentType()->isTypePointer();
}

bool SPIRVType::isTypeSubgroupAvcINTEL() const {
  return isSubgroupAvcINTELTypeOpCode(OpCode);
}

bool SPIRVType::isTypeSubgroupAvcMceINTEL() const {
  return OpCode == OpTypeAvcMcePayloadINTEL ||
         OpCode == OpTypeAvcMceResultINTEL;
}

bool SPIRVType::isTypeTaskSequenceINTEL() const {
  return OpCode == internal::OpTypeTaskSequenceINTEL;
}

bool SPIRVType::isTypeVectorOrScalarInt() const {
  return isTypeInt() || isTypeVectorInt();
}

bool SPIRVType::isTypeVectorOrScalarFloat() const {
  return isTypeFloat() || isTypeVectorFloat();
}

bool SPIRVType::isSPIRVOpaqueType() const {
  return isTypeDeviceEvent() || isTypeEvent() || isTypeImage() ||
         isTypePipe() || isTypeReserveId() || isTypeSampler() ||
         isTypeSampledImage() || isTypePipeStorage() ||
         isTypeCooperativeMatrixKHR() || isTypeJointMatrixINTEL() ||
         isTypeTaskSequenceINTEL();
}

bool SPIRVTypeStruct::isPacked() const {
  return hasDecorate(DecorationCPacked);
}

void SPIRVTypeStruct::setPacked(bool Packed) {
  if (Packed)
    addDecorate(new SPIRVDecorate(DecorationCPacked, this));
  else
    eraseDecorate(DecorationCPacked);
}

SPIRVTypeArray::SPIRVTypeArray(SPIRVModule *M, SPIRVId TheId,
                               SPIRVType *TheElemType, SPIRVValue *TheLength)
    : SPIRVType(M, 4, OpTypeArray, TheId), ElemType(TheElemType),
      Length(TheLength->getId()) {
  validate();
}

void SPIRVTypeArray::validate() const {
  SPIRVEntry::validate();
  ElemType->validate();
  assert(getValue(Length)->getType()->isTypeInt());
  assert(isConstantOpCode(getValue(Length)->getOpCode()));
}

SPIRVValue *SPIRVTypeArray::getLength() const { return getValue(Length); }

_SPIRV_IMP_ENCDEC3(SPIRVTypeArray, Id, ElemType, Length)

void SPIRVTypeForwardPointer::encode(spv_ostream &O) const {
  getEncoder(O) << PointerId << SC;
}

void SPIRVTypeForwardPointer::decode(std::istream &I) {
  auto Decoder = getDecoder(I);
  Decoder >> PointerId >> SC;
}

SPIRVTypeJointMatrixINTEL::SPIRVTypeJointMatrixINTEL(
    SPIRVModule *M, SPIRVId TheId, Op OC, SPIRVType *CompType,
    std::vector<SPIRVValue *> Args)
    : SPIRVType(M, FixedWC + Args.size(), OC, TheId), CompType(CompType),
      Args(std::move(Args)) {}

SPIRVTypeJointMatrixINTEL::SPIRVTypeJointMatrixINTEL(
    SPIRVModule *M, SPIRVId TheId, SPIRVType *CompType,
    std::vector<SPIRVValue *> Args)
    : SPIRVType(M, FixedWC + Args.size(), internal::OpTypeJointMatrixINTEL,
                TheId),
      CompType(CompType), Args(std::move(Args)) {}

SPIRVTypeJointMatrixINTEL::SPIRVTypeJointMatrixINTEL()
    : SPIRVType(internal::OpTypeJointMatrixINTEL), CompType(nullptr),
      Args({nullptr, nullptr, nullptr, nullptr}) {}

void SPIRVTypeJointMatrixINTEL::encode(spv_ostream &O) const {
  auto Encoder = getEncoder(O);
  Encoder << Id << CompType << Args;
}

void SPIRVTypeJointMatrixINTEL::decode(std::istream &I) {
  auto Decoder = getDecoder(I);
  Decoder >> Id >> CompType >> Args;
}

SPIRVTypeCooperativeMatrixKHR::SPIRVTypeCooperativeMatrixKHR(
    SPIRVModule *M, SPIRVId TheId, SPIRVType *CompType,
    std::vector<SPIRVValue *> Args)
    : SPIRVType(M, FixedWC, OpTypeCooperativeMatrixKHR, TheId),
      CompType(CompType), Args(std::move(Args)) {}

SPIRVTypeCooperativeMatrixKHR::SPIRVTypeCooperativeMatrixKHR()
    : SPIRVType(OpTypeCooperativeMatrixKHR), CompType(nullptr),
      Args({nullptr, nullptr, nullptr, nullptr}) {}

void SPIRVTypeCooperativeMatrixKHR::encode(spv_ostream &O) const {
  auto Encoder = getEncoder(O);
  Encoder << Id << CompType << Args;
}

void SPIRVTypeCooperativeMatrixKHR::decode(std::istream &I) {
  auto Decoder = getDecoder(I);
  Decoder >> Id >> CompType >> Args;
}

void SPIRVTypeCooperativeMatrixKHR::validate() const {
  SPIRVEntry::validate();
  SPIRVErrorLog &SPVErrLog = this->getModule()->getErrorLog();
  SPIRVConstant *UseConst = static_cast<SPIRVConstant *>(this->getUse());
  auto InstName = OpCodeNameMap::map(OC);
  uint64_t UseValue = UseConst->getZExtIntValue();
  SPVErrLog.checkError(
      (UseValue <= CooperativeMatrixUseMatrixAccumulatorKHR),
      SPIRVEC_InvalidInstruction,
      InstName + "\nIncorrect Use parameter, should be MatrixA, MatrixB or "
                 "Accumulator\n");
  SPIRVConstant *ScopeConst = static_cast<SPIRVConstant *>(this->getScope());
  uint64_t ScopeValue = ScopeConst->getZExtIntValue();
  SPVErrLog.checkError((ScopeValue <= ScopeInvocation),
                       SPIRVEC_InvalidInstruction,
                       InstName + "\nUnsupported Scope parameter\n");
}

} // namespace SPIRV
