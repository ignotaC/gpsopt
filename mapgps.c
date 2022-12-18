

/*

 Copyright (c) 2022 Piotr Trzpil  p.trzpil@protonmail.com

 Permission to use, copy, modify, and distribute 
 this software for any purpose with or without fee
 is hereby granted, provided that the above copyright
 notice and this permission notice appear in all copies.

 THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR
 DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
 FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 OF THIS SOFTWARE.

*/



#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef NDEBUG
  #define DBGMSG( x ) ( fprintf( stderr, "%s", x ) )
  #define DBGPRI( ... ) ( fprintf( stderr, __VA_ARGS__ ) )
#else
  #define DBGMSG( x )
  #define DBGPRI( ... ) 
#endif

// We cut glob  in arc seconds
// files are made depending on position and positions are stored there
#define WORLD_ARC 60

// HARDCODED MAX METER RADIOUS
int max_meterradius = 20;

void fail( const char *const estr )  {

  perror( estr );
  exit( EXIT_FAILURE );

}

#define SINGLE_POINT 1
#define POINT_NUMBER 3
struct point {

  double x;
  double y;
  double z;

};

int isdir( const char *const dirname )  {

  assert( dirname != NULL );
  int fd = open( dirname, O_DIRECTORY | O_RDONLY );
  if( fd == -1 )  return -1;
  close( fd );
  return 0;

}


// 1 = close
// 0 = far
int ispoint_close( const struct point *const chkpos,
		  const struct point *const pos1,
		  const struct point *const pos2 )  {

  /*
   
   Basic 3d line equation for two points
   
   ( x - x_1 ) / ( x_2 - x_1 ) =
   ( y - y_1 ) / ( y_2 - y_1 ) =
   ( z - z_1 ) / ( z_2 - z_1 )

   */

  assert( chkpos != NULL );
  assert( pos1 != NULL );
  assert( pos2 != NULL );

  DBGPRI( "checked_point = %f %f %f\n", chkpos->x, chkpos->y, chkpos->z );

  double l = pos2->x - pos1->x;
  double m = pos2->y - pos1->y;
  double n = pos2->z - pos1->z;

  DBGPRI( "l=%f m=%f n=%f\n", l, m, n );

  // Now find distance point M from a line that we created.

  // should we not check if points do not lie on same pos ?
  // at leats end up if we have a zero 

  double equation_denominator = l * l + m * m + n * n;
  DBGPRI( "equat_denominator=%f\n", equation_denominator );
  if( equation_denominator < 0.001 )  {

    // at this point it seems pos1 == pos2
    // so simply check if chkpos == pos1

    // should do (x-x2) ^2 TODO
    if( ( chkpos->x - pos1->x + chkpos->y - pos1->y +
      chkpos->z - pos1->z ) < 0.1 )  return 1;

    // else they are not close
    return 0;

  }


  double equation_numerator_1 = ( chkpos->x - pos1->x ) * m - ( chkpos->y - pos1->y ) * l;
  equation_numerator_1 *= equation_numerator_1;
  DBGPRI( "equat_numerator_1=%f\n", equation_numerator_1 );

  double equation_numerator_2 = ( chkpos->y - pos1->y ) * n - ( chkpos->z - pos1->z ) * m;
  equation_numerator_2 *= equation_numerator_2;
  DBGPRI( "equat_numerator_2=%f\n", equation_numerator_2 );

  double equation_numerator_3 = ( chkpos->z - pos1->z ) * l - ( chkpos->x - pos1->x ) * n; 
  equation_numerator_3 *= equation_numerator_3;
  DBGPRI( "equat_numerator_3=%f\n", equation_numerator_3 );

  double equation = equation_numerator_1 
      + equation_numerator_2
      + equation_numerator_3;
  DBGPRI( "equation=%f\n", equation );

  equation /= equation_denominator;
  DBGPRI( "equation=%f\n", equation );

  equation = sqrt( equation );
  DBGPRI( "equation=%f\n", equation );

  // finally if it's too far from line we don't need to do any more checks
  // if it's close there is chance point is in radious of our close points

  if(  equation > ( double ) max_meterradius )
    return 0;

  // this still needs checkup

  // now we need to know what is the absolute distance between the loaded points
  // and absolute distance between points and point we are checking is it close.
  double p1p2_abs_dist = ( pos1->x - pos2->x ) * ( pos1->x - pos2->x );
  p1p2_abs_dist += ( pos1->y - pos2->y ) * ( pos1->y - pos2->y );
  p1p2_abs_dist += ( pos1->z - pos2->z ) * ( pos1->z - pos2->z );
  p1p2_abs_dist = sqrt( p1p2_abs_dist );

  // we simply let the point to be a little bit close to count it as allready existing.
  double range = p1p2_abs_dist + 1; // here should be +1
   
  double p1chk_abs_dist = ( pos1->x - chkpos->x ) * ( pos1->x - chkpos->x );
  p1chk_abs_dist += ( pos1->y - chkpos->y ) * ( pos1->y - chkpos->y );
  p1chk_abs_dist += ( pos1->z - chkpos->z ) * ( pos1->z - chkpos->z );
  p1chk_abs_dist = sqrt( p1chk_abs_dist );

  double p2chk_abs_dist = ( chkpos->x - pos2->x ) * ( chkpos->x - pos2->x );
  p2chk_abs_dist += ( chkpos->y - pos2->y ) * ( chkpos->y - pos2->y );
  p2chk_abs_dist += ( chkpos->z - pos2->z ) * ( chkpos->z - pos2->z );
  p2chk_abs_dist = sqrt( p2chk_abs_dist );

  // we need the bigger value to know if we are aout of range.
  double longer_abs_dist = p1chk_abs_dist;
  if( p1chk_abs_dist < p2chk_abs_dist )
    longer_abs_dist = p2chk_abs_dist;

  // now we just find from equation that is radious and the abs distans
  // value of projection on 3d line that the two points create.

  double projection = longer_abs_dist * longer_abs_dist;
  projection -= equation * equation;
  projection = sqrt( projection );

  if( projection > range )  return 0;
  return 1;

}

#define GPSMAIN_DIR "gpstree/"
#define LONGTITUDE "lon"
#define ALTITUDE "alt"
int append_points( const struct point *const p1, 
    const struct point *const p2, const char *const gpsfile )  {

  assert( p1 != NULL );
  assert( p2 != NULL );
  assert( gpsfile != NULL );

  FILE *filegps = fopen( gpsfile, "a" );
  if( filegps == NULL )  return -1;

  int32_t worldpos = ( int32_t )p1->x;
  if( fwrite( &worldpos, sizeof worldpos, SINGLE_POINT, filegps ) != 
      SINGLE_POINT )
    return -1;
  worldpos = ( int32_t )p1->y;
  if( fwrite( &worldpos, sizeof worldpos, SINGLE_POINT, filegps ) != 
      SINGLE_POINT )
    return -1;
  worldpos = ( int32_t )p1->z;
  if( fwrite( &worldpos, sizeof worldpos, SINGLE_POINT, filegps ) != 
      SINGLE_POINT )
    return -1;
 

  worldpos = ( int32_t )p2->x;
  if( fwrite( &worldpos, sizeof worldpos, SINGLE_POINT, filegps ) != 
      SINGLE_POINT )
    return -1;
  worldpos = ( int32_t )p2->y;
  if( fwrite( &worldpos, sizeof worldpos, SINGLE_POINT, filegps ) != 
      SINGLE_POINT )
    return -1;
  worldpos = ( int32_t )p2->z;
  if( fwrite( &worldpos, sizeof worldpos, SINGLE_POINT, filegps ) != 
      SINGLE_POINT )
    return -1;
 

  fclose( filegps );
  return 0;

}

#define REMOVE_POINT 2
#define KEEP_POINT 0

#define PI_NUM  ( 3.141592 )

int main( const int argc, const char *const argv[] )  {

  // we expect two points
  if( argc != 7 )  fail( "Wrong number of arguments" );
  // And this is as far as we check we assume data is correct.
  // since we make the file, 

  DBGMSG( "\nmapgps start\n" );

  struct point np_1 = {0};
  np_1.x = strtod( argv[1], NULL );
  np_1.y = strtod( argv[2], NULL );
  np_1.z = strtod( argv[3], NULL );

  struct point np_2 = {0};
  np_2.x = strtod( argv[4], NULL );
  np_2.y = strtod( argv[5], NULL );
  np_2.z = strtod( argv[6], NULL );

  DBGPRI( "p1 = %.2f %.2f %.2f\n", np_1.x, np_1.y, np_1.z );
  DBGPRI( "p2 = %.2f %.2f %.2f\n", np_2.x, np_2.y, np_2.z );

  // we could not convert so remove the point
  if( errno )  return REMOVE_POINT;

  // At this point wee need to enter gps tree

#define DIRMOD ( S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH )
#define FILEMOD ( S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )

#define GPSFILEPATH_SIZE 8192
  char gpsfile[ GPSFILEPATH_SIZE ] = GPSMAIN_DIR;
  if( isdir( gpsfile ) == -1 )  {

    if( errno != ENOENT )  fail( "Failed on gpsmain path open" );
    if( mkdir( gpsfile, DIRMOD ) == -1 )
      fail( " Could not create gpsmain path dir" );

  }
 
  // X Y arc
  // arc is in seconds
  // // TODO bug in code
  double arc = atan( np_1.x / np_1.y ) * 180.0 / PI_NUM;
  arc *= 60.0 * 60.0;
  int arc_i = ( ( int )arc / WORLD_ARC ) * WORLD_ARC;
  sprintf( gpsfile, "%s%s%d/", GPSMAIN_DIR, LONGTITUDE, arc_i );

  if( isdir( gpsfile ) == -1 )  {

    DBGPRI( "Creating dir: %s\n", gpsfile );
    if( errno != ENOENT )  fail( "Failed on gpslon path open" );
    if( mkdir( gpsfile, DIRMOD ) == -1 )
      fail( " Could not create gpslon path dir" );

  }

  // XY Z arc
  double np_1xy = sqrt( np_1.x * np_1.x + np_2.y * np_2.y );
  double arc2 = atan( np_1.z / np_1xy ) * 180.0 / PI_NUM;
  arc2 *= 60.0 * 60.0;
  int arc2_i = ( ( int )arc2 / WORLD_ARC ) * WORLD_ARC;  
  
  sprintf( gpsfile, "%s%s%d/%s%d", GPSMAIN_DIR, LONGTITUDE, arc_i, ALTITUDE, arc2_i );

#define POINTS_TO_READ 6
  int32_t p_set[ POINTS_TO_READ ];

  FILE *binfile = fopen( gpsfile, "r" );
  if( binfile == NULL )  {

        DBGPRI( "Creating file: %s\n", gpsfile );
        if( append_points( &np_1, &np_2, gpsfile ) == -1 )
          fail( "Could not write new points to gpslines" );
	return KEEP_POINT;

  }

  DBGPRI( "Using file: %s\n", gpsfile );

  struct point tp_1 = {0};
  struct point tp_2 = {0};
  int ispoint_close_ret = 0;
  int np1_CLOSE = 0, np2_CLOSE = 0;
  for(;;)  {

    errno = 0;
    size_t fr_ret = fread( p_set, sizeof p_set[0], POINTS_TO_READ, binfile );
    if( fr_ret == POINTS_TO_READ )  { 

      tp_1.x = p_set[0];
      tp_1.y = p_set[1];
      tp_1.z = p_set[2];
      tp_2.x = p_set[3];
      tp_2.y = p_set[4];
      tp_2.z = p_set[5];

      DBGPRI( "Loaded p1: %.2f %.2f %.2f\n", tp_1.x, tp_1.y, tp_1.z );
      DBGPRI( "Loaded p2: %.2f %.2f %.2f\n", tp_2.x, tp_2.y, tp_2.z );

      // tu jest blad bo po znalezieniu jednego powinniśmy szukać drugiego.
      if( ! np1_CLOSE )  {

         ispoint_close_ret = ispoint_close( &np_1, &tp_1, &tp_2 );
         if( ispoint_close_ret )  np1_CLOSE = 1;

      }

      if( ! np2_CLOSE )  {
        
        ispoint_close_ret = ispoint_close( &np_2, &tp_1, &tp_2 );
	if( ispoint_close_ret )  np2_CLOSE = 1;

      }

      if( np1_CLOSE && np2_CLOSE )  {

	DBGMSG( "Both points in range" );
        fclose( binfile );
	return REMOVE_POINT;

      }

      continue;

    }

    // search for failure
    if( fr_ret != 0 )  fail( "Broken points to gpslines" );
    if( errno )  fail( "Error on fread function" );
    break;  // at this point we can leave loop

  }

  fclose( binfile );
  if( append_points( &np_1, &np_2, gpsfile ) == -1 )
    fail( "Could not write new points to gpslines" );
  return KEEP_POINT;

}
