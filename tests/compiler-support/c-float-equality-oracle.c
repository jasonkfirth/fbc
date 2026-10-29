/*
 * Project: FreeBASIC C comparison regression tests
 * File: c-float-equality-oracle.c
 * Purpose: Supply independent native C equality operations as the reference.
 * Responsibilities: Preserve C results and floating exception behavior.
 * This file intentionally does NOT use the emitter's comparison predicates.
 * The six exact reference operations intentionally produce -Wfloat-equal;
 * the generated candidate is checked separately and must have no warnings.
 */
int fb_eq_base_d_eq( double lhs, double rhs );
int fb_eq_base_d_ne( double lhs, double rhs );
int fb_eq_base_s_eq( float lhs, float rhs );
int fb_eq_base_s_ne( float lhs, float rhs );
int fb_eq_base_d_bool( double value );
int fb_eq_base_d_effect( void );
double fb_eq_operand_d( int which );

int fb_eq_base_d_eq( double lhs, double rhs )
{
	return -(lhs == rhs);
}

int fb_eq_base_d_ne( double lhs, double rhs )
{
	return -(lhs != rhs);
}

int fb_eq_base_s_eq( float lhs, float rhs )
{
	return -(lhs == rhs);
}

int fb_eq_base_s_ne( float lhs, float rhs )
{
	return -(lhs != rhs);
}

int fb_eq_base_d_bool( double value )
{
	return -(value != 0);
}

int fb_eq_base_d_effect( void )
{
	return -(fb_eq_operand_d(0) == fb_eq_operand_d(1));
}

/* end of c-float-equality-oracle.c */
