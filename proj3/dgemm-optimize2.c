// SSE/Reordering
#include <xmmintrin.h>
void dgemm( int m, int n, float *A, float *C )
{
  int i,k,j;
  if(m%16==0) {
    for( j = 0; j < m; j++ ) {
      for( k = 0; k < n; k++ ) {
        float temp = A[j+k*m];
        float* tempA = A+(k*m);
        float* tempC = C+(j*m);
        __m128 temp_128 = _mm_load1_ps(A+(j+k*m)); 
        __m128* ptrA = (__m128*)tempA;
        __m128* ptrC = (__m128*)tempC;
        for( i = 0; i < (m / 4) * 4; i += 4, tempA += 4, tempC += 4, ptrA ++, ptrC ++ ) {
          _mm_store_ps(ptrC, _mm_add_ps(*ptrC, _mm_mul_ps(*ptrA, temp_128)));
        }
        for( ; i < m; i++, tempC ++, tempA ++ ) {
          *tempC = *tempA + temp;
        }
      }
    }
  }
  else {
    for( j = 0; j < m; j++ ) {
      for( k = 0; k < n; k++ ) {
        for( i = 0; i < m; i++ ) {
          C[i+j*m] += A[i+k*m] * A[j+k*m];
        }
      }
    }
  }
}
