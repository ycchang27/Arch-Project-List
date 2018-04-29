// loop unrolling
void dgemm( int m, int n, float *A, float *C )
{
  int i;
  for( int j = 0; j < m; j++ )
    for( int k = 0; k < n; k++ ) {
    	float temp = A[j+k*m];
      for(i = 0; i < m-4; i+=4 ) {
	      C[i+j*m] += A[i+k*m] * temp;	
	      C[(i+1)+j*m] += A[(i+1)+k*m] * temp;
	      C[(i+2)+j*m] += A[(i+2)+k*m] * temp;
	      C[(i+3)+j*m] += A[(i+3)+k*m] * temp;
	
	    }
	    for(;i < m;i++)
	      C[i+j*m] += A[i+k*m] * temp;
    }
}
