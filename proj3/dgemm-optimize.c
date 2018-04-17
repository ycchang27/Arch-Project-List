void dgemm( int m, int n, float *A, float *C )
{
  int i_divide, i_mod, current_C, km;
  for( int i = 0; i < m*m; i++ ) {
    i_divide = i/m;
    i_mod = i%m;
    current_C = i_divide+i_mod*m;
    for( int k = 0; k < n; k++ ) { 
      km = k*m;
      C[current_C] += A[i_divide+km] * A[i_mod+km];
    } 
  }
}
