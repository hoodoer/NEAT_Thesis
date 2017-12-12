#ifndef CVec3_h
#define CVec3_h

class CVec3
{
 public:
  // Data
  double x, y, z;

  // Ctors
  CVec3( double InX, double InY, double InZ ) : x( InX ), y( InY ), z( InZ )
    {
    }
  CVec3( ) : x(0), y(0), z(0)
    {
    }

  // Operator Overloads
  inline bool operator== (const CVec3& V2) const 
    {
      return (x == V2.x && y == V2.y && z == V2.z);
    }

  inline CVec3 operator+ (const CVec3& V2) const 
    {
      return CVec3( x + V2.x,  y + V2.y,  z + V2.z);
    }
  inline CVec3 operator- (const CVec3& V2) const
    {
      return CVec3( x - V2.x,  y - V2.y,  z - V2.z);
    }
  inline CVec3 operator- ( ) const
    {
      return CVec3(-x, -y, -z);
    }

  inline CVec3 operator/ (double S ) const
    {
      double fInv = 1.0f / S;
      return CVec3 (x * fInv , y * fInv, z * fInv);
    }
  inline CVec3 operator/ (const CVec3& V2) const
    {
      return CVec3 (x / V2.x,  y / V2.y,  z / V2.z);
    }
  inline CVec3 operator* (const CVec3& V2) const
    {
      return CVec3 (x * V2.x,  y * V2.y,  z * V2.z);
    }
  inline CVec3 operator* (double S) const
    {
      return CVec3 (x * S,  y * S,  z * S);
    }

  inline void operator+= ( const CVec3& V2 )
    {
      x += V2.x;
      y += V2.y;
      z += V2.z;
    }
  inline void operator-= ( const CVec3& V2 )
    {
      x -= V2.x;
      y -= V2.y;
      z -= V2.z;
    }

  // Functions
  inline double Dot( const CVec3 &V1 ) const
    {
      return V1.x*x + V1.y*y + V1.z*z;
    }

  inline CVec3 CrossProduct( const CVec3 &V2 ) const
    {
      return CVec3(
		   y * V2.z  -  z * V2.y,
		   z * V2.x  -  x * V2.z,
		   x * V2.y  -  y * V2.x 	);
    }

  // These require math.h for the sqrtf function
  double Magnitude( ) const
    {
      return sqrtf( x*x + y*y + z*z );
    }

  double Distance( const CVec3 &V1 ) const
    {
      return ( *this - V1 ).Magnitude();	
    }

  inline void Normalize()
    {
      double fMag = ( x*x + y*y + z*z );
      if (fMag == 0) {return;}

      double fMult = 1.0f/sqrtf(fMag);            
      x *= fMult;
      y *= fMult;
      z *= fMult;
      return;
    }
};
#endif
