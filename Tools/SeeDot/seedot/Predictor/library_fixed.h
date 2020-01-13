// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

//#define SATURATE
//#define FASTAPPROX
#define FLOATEXP

void MatAddNN(MYINT *A, MYINT *B, MYINT *C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC);
void MatAddCN(const MYINT *A, MYINT *B, MYINT *C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC);
void MatAddNC(MYINT *A, const MYINT *B, MYINT *C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC);
void MatAddCC(const MYINT *A, const MYINT *B, MYINT *C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC);

void MatAddBroadCastA(MYINT *A, MYINT *B, MYINT *C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC);
void MatAddBroadCastB(MYINT *A, MYINT *B, MYINT *C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC);

void MatSub(MYINT *A, const MYINT *B, MYINT *C, MYINT I, MYINT J, MYINT shrA, int32_t shrB, MYINT shrC);
void MatSubBroadCastA(MYINT *A, MYINT *B, MYINT *C, MYINT I, MYINT J, MYINT shrA, int32_t shrB, MYINT shrC);
void MatSubBroadCastB(MYINT *A, MYINT *B, MYINT *C, MYINT I, MYINT J, MYINT shrA, int32_t shrB, MYINT shrC);

void MatMulNN(MYINT *A, MYINT *B, MYINT *C, MYINT *tmp, MYINT I, MYINT K, MYINT J, MYINT shrA, MYINT shrB, MYINT H1, MYINT H2);

void MatMulCN(const MYINT *A, MYINT *B, MYINT *C, MYINT *tmp, MYINT I, MYINT K, MYINT J, MYINT shrA, MYINT shrB, MYINT H1, MYINT H2);

void MatMulNC(MYINT *A, const MYINT *B, MYINT *C, MYINT *tmp, MYINT I, MYINT K, MYINT J, MYINT shrA, MYINT shrB, MYINT H1, MYINT H2);

void MatMulCC(const MYINT *A, const MYINT *B, MYINT *C, MYINT *tmp, MYINT I, MYINT K, MYINT J, MYINT shrA, MYINT shrB, MYINT H1, MYINT H2);

void SparseMatMul(const MYINT *Aidx, const MYINT *Aval, MYINT **B, MYINT *C, int16_t K, MYINT shrA, MYINT shrB, MYINT shrC);

void MulCir(MYINT *A, MYINT *B, MYINT *C, MYINT I, MYINT J, MYINT shrA, MYINT shrB);

void TanH(MYINT *A, MYINT I, MYINT J, MYINT scale_in, MYINT scale_out);

void ArgMax(MYINT *A, MYINT I, MYINT J, MYINT *index);

void Transpose(MYINT *A, MYINT *B, MYINT I, MYINT J);

void ScalarMul(MYINT *A, MYINT *B, MYINT *C, MYINT I, MYINT J, MYINT shrA, MYINT shrB);

void Conv(MYINT *A, const MYINT *B, MYINT *C, MYINT *tmp, MYINT N, MYINT H, MYINT W, MYINT CI, MYINT HF, MYINT WF, MYINT CO, MYINT shrA, MYINT shrB, MYINT H1, MYINT H2);

void AddOrSubCir4D(MYINT *A, const MYINT *B, MYINT N, MYINT H, MYINT W, MYINT C, MYINT shrA, MYINT shrB, MYINT shrC, bool add);

void AddOrSubCir2D(MYINT *A, const MYINT *B, MYINT H, MYINT W, MYINT shrA, MYINT shrB, MYINT shrC, bool add);

void Relu4D(MYINT *A, MYINT N, MYINT H, MYINT W, MYINT C);

void Relu2D(MYINT *A, MYINT H, MYINT W);

void Maxpool(MYINT *A, MYINT *B, MYINT N, MYINT H, MYINT W, MYINT C, MYINT stride);

void Exp(MYINT *A, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT *B);

void Sigmoid(MYINT *A, MYINT I, MYINT J, MYINT div, MYINT add, MYINT sigmoid_limit, MYINT scale_in, MYINT scale_out);

void AdjustScaleShr(MYINT *A, MYINT I, MYINT J, MYINT scale);
void AdjustScaleShl(MYINT *A, MYINT I, MYINT J, MYINT scale);

//Templated Operations: For cases when Variable BitWidth is enabled

template<class TypeA>
inline TypeA Saturate(int32_t inp) {
	return (TypeA)inp;
}

template<>
inline int16_t Saturate(int32_t inp) {
#ifdef SATURATE
	return (int16_t)((inp > 32767 ? 32767 : inp) < -32768 ? -32768 : inp);
#else
	return (int16_t)inp;
#endif
}

template<>
inline int8_t Saturate(int32_t inp) {
#ifdef SATURATE
	return (int8_t)((inp > 127 ? 127 : inp) < -128 ? -128 : inp);
#else
	return (int8_t)inp;
#endif
}

template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatAddNN(TypeA* A, TypeB* B, TypeC* C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeTemp a = (TypeTemp)A[i * J + j];
			TypeTemp b = (TypeTemp)B[i * J + j];

			a = a / shrA;
			b = b / shrB;

			TypeTemp c = a / shrC + b / shrC;

			C[i * J + j] = Saturate<TypeC>(c / demote);
		}
	}
	return;
}
template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatAddCN(const TypeA* A, TypeB* B, TypeC* C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeTemp a = (TypeTemp)A[i * J + j];
			TypeTemp b = (TypeTemp)B[i * J + j];

			a = a / shrA;
			b = b / shrB;

			TypeTemp c = a / shrC + b / shrC;

			C[i * J + j] = Saturate<TypeC>(c / demote);
		}
	}
	return;
}
template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatAddNC(TypeA* A, const TypeB* B, TypeC* C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeTemp a = (TypeTemp)A[i * J + j];
			TypeTemp b = (TypeTemp)B[i * J + j];

			a = a / shrA;
			b = b / shrB;

			TypeTemp c = a / shrC + b / shrC;

			C[i * J + j] = Saturate<TypeC>(c / demote);
		}
	}
	return;
}
template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatAddCC(const TypeA* A, const TypeB* B, TypeC* C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeTemp a = (TypeTemp)A[i * J + j];
			TypeTemp b = (TypeTemp)B[i * J + j];

			a = a / shrA;
			b = b / shrB;

			TypeTemp c = a / shrC + b / shrC;

			C[i * J + j] = Saturate<TypeC>(c / demote);
		}
	}
	return;
}

template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatAddBroadCastA(TypeA* A, TypeB* B, TypeC* C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeTemp a = (TypeTemp)* A;
			TypeTemp b = (TypeTemp)B[i * J + j];

			a = a / shrA;
			b = b / shrB;

			TypeTemp c = a / shrC + b / shrC;

			C[i * J + j] = Saturate<TypeC>(c / demote);
		}
	}
	return;
}
template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatAddBroadCastB(TypeA* A, TypeB* B, TypeC* C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeTemp a = (TypeTemp)A[i * J + j];
			TypeTemp b = (TypeTemp)* B;

			a = a / shrA;
			b = b / shrB;

			TypeTemp c = a / shrC + b / shrC;

			C[i * J + j] = Saturate<TypeC>(c / demote);
		}
	}
	return;
}

template<class TypeA, class TypeB, class TypeTemp, class TypeC>
// TODO: shrB is int32_t because in 8-bit/16-bit code, shrB is usually very high and int8_t/int16_t will overflow.
void MatSub(TypeA* A, const TypeB* B, TypeC* C, MYINT I, MYINT J, MYINT shrA, int32_t shrB, MYINT shrC, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeTemp a = (TypeTemp)A[i * J + j];
			TypeTemp b = (TypeTemp)B[i * J + j];

			a = a / shrA;
			b = b / shrB;

			MYINT c = a / shrC - b / shrC;

			C[i * J + j] = Saturate<TypeC>(c / demote);
		}
	}
	return;
}
template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatSubBroadCastA(TypeA* A, TypeB* B, TypeC* C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeTemp a = (TypeTemp)* A;
			TypeTemp b = (TypeTemp)B[i * J + j];

			a = a / shrA;
			b = b / shrB;

			TypeTemp c = a / shrC - b / shrC;

			C[i * J + j] = Saturate<TypeC>(c / demote);
		}
	}
	return;
}
template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatSubBroadCastB(TypeA* A, TypeB* B, TypeC* C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT shrC, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeTemp a = (TypeTemp)A[i * J + j];
			TypeTemp b = (TypeTemp)* B;

			a = a / shrA;
			b = b / shrB;

			TypeTemp c = a / shrC - b / shrC;

			C[i * J + j] = Saturate<TypeC>(c / demote);
		}
	}
	return;
}

template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatMulNN(TypeA* A, TypeB* B, TypeC* C, TypeTemp* tmp, MYINT I, MYINT K, MYINT J, MYINT shrA, MYINT shrB, MYINT H1, MYINT H2, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			for (MYITE k = 0; k < K; k++) {
				TypeTemp a = (TypeTemp)A[i * K + k];
				TypeTemp b = (TypeTemp)B[k * J + j];

				TypeTemp prod = a * b;

				tmp[k] = prod;
			}

			MYITE count = K, depth = 0;
			bool shr = true;

			while (depth < (H1 + H2)) {
				if (depth >= H1)
					shr = false;

				for (MYITE p = 0; p < (K / 2 + 1); p++) {
					TypeTemp sum;
					if (p < (count >> 1)) {
						if (shr)
							sum = tmp[2 * p] / 2 + tmp[(2 * p) + 1] / 2;
						else
							sum = tmp[2 * p] + tmp[(2 * p) + 1];
					}
					else if ((p == (count >> 1)) && ((count & 1) == 1)) {
						if (shr)
							sum = tmp[2 * p] / 2;
						else
							sum = tmp[2 * p];
					}
					else
						sum = 0;

					tmp[p] = sum;
				}
				count = (count + 1) >> 1;

				depth++;
			}

			C[i * J + j] = Saturate<TypeC>(((tmp[0] / shrA) / shrB) / demote);
		}
	}
	return;
}
template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatMulCN(const TypeA* A, TypeB* B, TypeC* C, TypeTemp* tmp, MYINT I, MYINT K, MYINT J, MYINT shrA, MYINT shrB, MYINT H1, MYINT H2, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			for (MYITE k = 0; k < K; k++) {
				TypeTemp a = (TypeTemp)A[i * K + k];
				TypeTemp b = (TypeTemp)B[k * J + j];

				TypeTemp prod = a * b;

				tmp[k] = prod;
			}

			MYITE count = K, depth = 0;
			bool shr = true;

			while (depth < (H1 + H2)) {
				if (depth >= H1)
					shr = false;

				for (MYITE p = 0; p < (K / 2 + 1); p++) {
					TypeTemp sum;
					if (p < (count >> 1)) {
						if (shr)
							sum = tmp[2 * p] / 2 + tmp[(2 * p) + 1] / 2;
						else
							sum = tmp[2 * p] + tmp[(2 * p) + 1];
					}
					else if ((p == (count >> 1)) && ((count & 1) == 1)) {
						if (shr)
							sum = tmp[2 * p] / 2;
						else
							sum = tmp[2 * p];
					}
					else
						sum = 0;

					tmp[p] = sum;
				}
				count = (count + 1) >> 1;

				depth++;
			}

			C[i * J + j] = Saturate<TypeC>(((tmp[0] / shrA) / shrB) / demote);
		}
	}
	return;
}
template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatMulNC(TypeA* A, const TypeB* B, TypeC* C, TypeTemp* tmp, MYINT I, MYINT K, MYINT J, MYINT shrA, MYINT shrB, MYINT H1, MYINT H2, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			for (MYITE k = 0; k < K; k++) {
				TypeTemp a = (TypeTemp)A[i * K + k];
				TypeTemp b = (TypeTemp)B[k * J + j];

				TypeTemp prod = a * b;

				tmp[k] = prod;
			}

			MYITE count = K, depth = 0;
			bool shr = true;

			while (depth < (H1 + H2)) {
				if (depth >= H1)
					shr = false;

				for (MYITE p = 0; p < (K / 2 + 1); p++) {
					TypeTemp sum;
					if (p < (count >> 1)) {
						if (shr)
							sum = tmp[2 * p] / 2 + tmp[(2 * p) + 1] / 2;
						else
							sum = tmp[2 * p] + tmp[(2 * p) + 1];
					}
					else if ((p == (count >> 1)) && ((count & 1) == 1)) {
						if (shr)
							sum = tmp[2 * p] / 2;
						else
							sum = tmp[2 * p];
					}
					else
						sum = 0;

					tmp[p] = sum;
				}
				count = (count + 1) >> 1;

				depth++;
			}

			C[i * J + j] = Saturate<TypeC>(((tmp[0] / shrA) / shrB) / demote);
		}
	}
	return;
}
template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MatMulCC(const TypeA* A, const TypeB* B, TypeC* C, TypeTemp* tmp, MYINT I, MYINT K, MYINT J, MYINT shrA, MYINT shrB, MYINT H1, MYINT H2, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			for (MYITE k = 0; k < K; k++) {
				TypeTemp a = (TypeTemp)A[i * K + k];
				TypeTemp b = (TypeTemp)B[k * J + j];

				TypeTemp prod = a * b;

				tmp[k] = prod;
			}

			MYITE count = K, depth = 0;
			bool shr = true;

			while (depth < (H1 + H2)) {
				if (depth >= H1)
					shr = false;

				for (MYITE p = 0; p < (K / 2 + 1); p++) {
					TypeTemp sum;
					if (p < (count >> 1)) {
						if (shr)
							sum = tmp[2 * p] / 2 + tmp[(2 * p) + 1] / 2;
						else
							sum = tmp[2 * p] + tmp[(2 * p) + 1];
					}
					else if ((p == (count >> 1)) && ((count & 1) == 1)) {
						if (shr)
							sum = tmp[2 * p] / 2;
						else
							sum = tmp[2 * p];
					}
					else
						sum = 0;

					tmp[p] = sum;
				}
				count = (count + 1) >> 1;

				depth++;
			}

			C[i * J + j] = Saturate<TypeC>(((tmp[0] / shrA) / shrB) / demote);
		}
	}
	return;
}

template<class TypeA, class TypeAidx, class TypeB, class TypeTemp, class TypeC>
void SparseMatMul(const TypeAidx* Aidx, const TypeA* Aval, TypeB** B, TypeC* C, int16_t K, MYINT shrA, MYINT shrB, MYINT shrC, MYINT demote) {

	MYITE ite_idx = 0, ite_val = 0;
	for (MYITE k = 0; k < K; k++) {
		// MYINT b = getIntFeature(k);
		TypeTemp b = (TypeTemp)B[k * 1][0];
		//b = b / shrB;

		MYINT idx = Aidx[ite_idx];
		while (idx != 0) {
			TypeTemp a = (TypeTemp)Aval[ite_val];
			//a = a / shrA;
			TypeTemp c = (TypeTemp)(a * b);
			//c = c / shrC;

			C[idx - 1] += Saturate<TypeC>((((c / shrA) / shrB) / shrC) / demote);

			ite_idx++;
			ite_val++;

			idx = Aidx[ite_idx];
		}
		ite_idx++;
	}

	return;
}
template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void MulCir(TypeA* A, TypeB* B, TypeC* C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeTemp a = (TypeTemp)A[i * J + j];
			TypeTemp b = (TypeTemp)B[i * J + j];

			TypeTemp prod = a * b;
			C[i * J + j] = Saturate<TypeC>(((prod / shrA) / shrB) / demote);
		}
	}
	return;
}

template<class TypeA>
void TanH(TypeA* A, MYINT I, MYINT J, TypeA tanh_limit) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			float x = float(A[i * J + j]) / tanh_limit;

			float y = tanh(x);

			MYINT z = (TypeA)(y * tanh_limit);

			A[i * J + j] = z;
		}
	}
	return;
}

template<class TypeA>
void Confidence(TypeA* A, float* confidence) {
	*confidence = *A;
	if (*confidence < 0)
		* confidence = -(*confidence);
}

template<class TypeA>
void Confidence(TypeA* A, MYINT I, MYINT J, MYITE* index, float* confidence) {
	TypeA max = A[0];
	TypeA min = A[0];
	MYITE maxIndex = 0, counter = 0;
	float sum = 0;
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeA x = A[i * J + j];
			//sum += x;
			if (max < x) {
				maxIndex = counter;
				max = x;
			}
			if (min > x) {
				min = x;
			}
			counter++;
		}
	}

	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			sum += (A[i * J + j] - min);
		}
	}

	*index = maxIndex;
	if (sum < 0.0001 && sum > -0.0001)
		* confidence = ((float)1) / (I * J); //Maybe could penalise more as this is a underflow
	else
		*confidence = (float)(A[*index]-min) / (sum);
	return;
}


template<class TypeA>
void ArgMax(TypeA* A, MYINT I, MYINT J, MYITE* index) {
	TypeA max = A[0];
	MYITE maxIndex = 0, counter = 0;
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeA x = A[i * J + j];

			if (max < x) {
				maxIndex = counter;
				max = x;
			}

			counter++;
		}
	}

	*index = maxIndex;

	return;
}

template<class TypeA>
void Transpose(TypeA* A, TypeA* B, MYINT I, MYINT J) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			B[i * J + j] = A[j * I + i];
		}
	}
	return;
}

template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void ScalarMul(TypeA* A, TypeB* B, TypeC* C, MYINT I, MYINT J, MYINT shrA, MYINT shrB, int demote) {
	TypeTemp a = (TypeTemp)* A;
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeTemp b = (TypeTemp)B[i * J + j];

			TypeTemp prod = a * b;
			C[i * J + j] = Saturate<TypeC>(((prod / shrA) / shrB) / demote);
		}
	}

	return;
}

template<class TypeA, class TypeB, class TypeTemp, class TypeC>
void Conv(TypeA* A, const TypeB* B, TypeC* C, TypeTemp* tmp, MYINT N, MYINT H, MYINT W, MYINT CI, MYINT HF, MYINT WF, MYINT CO, MYINT shrA, MYINT shrB, MYINT H1, MYINT H2, MYINT demote) {
	MYITE padH = (HF - 1) / 2;
	MYITE padW = (WF - 1) / 2;

	for (MYITE n = 0; n < N; n++) {
		for (MYITE h = 0; h < H; h++) {
			for (MYITE w = 0; w < W; w++) {
				for (MYITE co = 0; co < CO; co++) {

					MYITE counter = 0;
					for (MYITE hf = 0; hf < HF; hf++) {
						for (MYITE wf = 0; wf < WF; wf++) {
							for (MYITE ci = 0; ci < CI; ci++) {
								TypeTemp a = (TypeTemp)(((((h + hf) < padH) || ((h + hf) >= (H + padH))) || (((w + wf) < padW) || ((w + wf) >= (W + padW)))) ? 0 : A[n * H * W * CI + ((h + hf) - padH) * W * CI + ((w + wf) - padW) * CI + ci]);

								TypeTemp b = (TypeTemp)B[hf * WF * CI * CO + wf * CI * CO + ci * CO + co];

								tmp[counter] = a * b;
								counter++;
							}
						}
					}

					MYITE totalEle = HF * WF * CI;
					MYITE count = HF * WF * CI, depth = 0;
					bool shr = true;

					while (depth < (H1 + H2)) {
						if (depth >= H1)
							shr = false;

						for (MYITE p = 0; p < (totalEle / 2 + 1); p++) {
							TypeTemp sum;
							if (p < (count >> 1)) {
								if (shr)
									sum = tmp[2 * p] / 2 + tmp[(2 * p) + 1] / 2;
								else
									sum = tmp[2 * p] + tmp[(2 * p) + 1];
							}
							else if ((p == (count >> 1)) && ((count & 1) == 1)) {
								if (shr)
									sum = tmp[2 * p] / 2;
								else
									sum = tmp[2 * p];
							}
							else
								sum = 0;

							tmp[p] = sum;
						}
						count = (count + 1) >> 1;

						depth++;
					}

					C[n * H * W * CO + h * W * CO + w * CO + co] = Saturate<TypeC>(((tmp[0] / shrA) / shrB) / demote);
				}
			}
		}
	}
	return;
}

template<class TypeA, class TypeB, class TypeTemp>
void AddOrSubCir4D(TypeA* A, const TypeB* B, MYINT N, MYINT H, MYINT W, MYINT C, MYINT shrA, MYINT shrB, MYINT shrC, bool add) {
	for (MYITE n = 0; n < N; n++) {
		for (MYITE h = 0; h < H; h++) {
			for (MYITE w = 0; w < W; w++) {
				for (MYITE c = 0; c < C; c++) {
					TypeTemp a = (TypeTemp)A[n * H * W * C + h * W * C + w * C + c];
					a = a / shrA;

					TypeTemp b = (TypeTemp)B[c];
					b = b / shrB;

					TypeTemp res;
					if (add)
						res = a / shrC + b / shrC;
					else
						res = a / shrC - b / shrC;

					A[n * H * W * C + h * W * C + w * C + c] = Saturate<TypeA>(res);
				}
			}
		}
	}
	return;
}
template<class TypeA, class TypeB, class TypeTemp>
void AddOrSubCir2D(TypeA* A, const TypeB* B, MYINT H, MYINT W, MYINT shrA, MYINT shrB, MYINT shrC, bool add) {
	for (MYITE h = 0; h < H; h++) {
		for (MYITE w = 0; w < W; w++) {
			TypeTemp a = (TypeTemp)A[h * W + w];
			a = a / shrA;

			TypeTemp b = (TypeTemp)B[w];
			b = b / shrB;

			TypeTemp res;
			if (add)
				res = a / shrC + b / shrC;
			else
				res = a / shrC - b / shrC;

			A[h * W + w] = Saturate<TypeA>(res);
		}
	}

	return;
}

template<class TypeA>
void Relu4D(TypeA* A, MYINT N, MYINT H, MYINT W, MYINT C) {
	for (MYITE n = 0; n < N; n++) {
		for (MYITE h = 0; h < H; h++) {
			for (MYITE w = 0; w < W; w++) {
				for (MYITE c = 0; c < C; c++) {
					TypeA a = A[n * H * W * C + h * W * C + w * C + c];
					if (a < 0)
						a = 0;
					A[n * H * W * C + h * W * C + w * C + c] = a;
				}
			}
		}
	}
	return;
}
template<class TypeA>
void Relu2D(TypeA* A, MYINT H, MYINT W) {
	for (MYITE h = 0; h < H; h++) {
		for (MYITE w = 0; w < W; w++) {
			TypeA a = A[h * W + w];
			if (a < 0)
				a = 0;
			A[h * W + w] = a;
		}
	}
	return;
}
template<class TypeA, class TypeB>
void Maxpool(TypeA* A, TypeB* B, MYINT N, MYINT H, MYINT W, MYINT C, MYINT stride, MYINT demote) {
	MYITE HO = H / stride;
	MYITE WO = W / stride;

	for (MYITE n = 0; n < N; n++) {
		for (MYITE ho = 0; ho < HO; ho++) {
			for (MYITE wo = 0; wo < WO; wo++) {
				for (MYITE c = 0; c < C; c++) {

					TypeA max = A[n * H * W * C + (stride * ho) * W * C + (stride * wo) * C + c];
					for (MYITE hs = 0; hs < stride; hs++) {
						for (MYITE ws = 0; ws < stride; ws++) {
							TypeA a = A[n * H * W * C + ((stride * ho) + hs) * W * C + ((stride * wo) + ws) * C + c];
							if (a > max)
								max = a;
						}
					}

					B[n * HO * WO * C + ho * WO * C + wo * C + c] = (TypeB)(max / demote);
				}
			}
		}
	}
	return;
}

//shrB overflows int16_t
template<class TypeA, class TypeB>
void Exp(TypeA* A, MYINT I, MYINT J, MYINT shrA, int32_t shrB, TypeB* B, MYINT demote) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			B[i * J + j] = (TypeB)((exp(((float)A[i * J + j]) / shrA) * shrB) / demote);
		}
	}
	return;
}
template<class TypeA>
void Sigmoid(TypeA* A, MYINT I, MYINT J, MYINT div, MYINT add, MYINT sigmoid_limit, MYINT scale_in, MYINT scale_out) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			float x = float(A[i * J + j]) / scale_in;

			float y = 1 / (1 + exp(-x));

			TypeA z = (TypeA)(y * scale_out);

			A[i * J + j] = z;
		}
	}

	return;
}

template<class TypeA>
void AdjustScaleShr(TypeA* A, MYINT I, MYINT J, MYINT scale) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeA a = A[i * J + j];
			A[i * J + j] = a / scale;
		}
	}
	return;
}
template<class TypeA>
void AdjustScaleShl(TypeA* A, MYINT I, MYINT J, MYINT scale) {
	for (MYITE i = 0; i < I; i++) {
		for (MYITE j = 0; j < J; j++) {
			TypeA a = A[i * J + j];
			A[i * J + j] = a * scale;
		}
	}
	return;
}
