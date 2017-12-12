#ifndef lineIntersect2D_h
#define lineIntersect2D_h

#include <algorithm>

struct Point2D
{
  float x;
  float y;
};

struct Line2D
{
  float x1;
  float y1;
  float x2;
  float y2;
};



// Returns true if the lines intersect, and intersectPoint
// is set to the point of intersection. Returns false if 
// the lines do not intersect
inline bool intersect2dLines(Line2D firstLine, Line2D secondLine, 
			     Point2D &intersectPoint)
{
    float d = (firstLine.x1 - firstLine.x2) *
            (secondLine.y1 - secondLine.y2) -
            (firstLine.y1  - firstLine.y2)   *
            (secondLine.x1 - secondLine.x2);


    // If do is zero, there is no intersection
    if (d == 0)
    {
        intersectPoint.x = 0.0;
        intersectPoint.y = 0.0;
        return false;
    }

    // Figure out new x and y
    float pre  = (firstLine.x1 * firstLine.y2 - firstLine.y1 * firstLine.x2);
    float post = (secondLine.x1 * secondLine.y2 - secondLine.y1 * secondLine.x2);
    float x    = (pre * ( secondLine.x1 - secondLine.x2) -
                  (firstLine.x1 - firstLine.x2) * post)/d;
    float y    = (pre * (secondLine.y1 - secondLine.y2) -
                  (firstLine.y1 - firstLine.y2) * post)/d;


    // Helps with testing against horizontal or
    // vertical lines, which can incorrectly fail
    // due to floating point rounding errors.
    // Effectively tests against a line with a
    // thickness of 2*epsilon
    float epsilon = 0.0001;


    // check to make sure intersect x & y are within both lines
    if ((x < min(firstLine.x1, firstLine.x2)       - epsilon) ||
            (x > max(firstLine.x1, firstLine.x2)   + epsilon) ||
            (x < min(secondLine.x1, secondLine.x2) - epsilon) ||
            (x > max(secondLine.x1, secondLine.x2) + epsilon))
    {
        // It's not
        intersectPoint.x = 0.0;
        intersectPoint.y = 0.0;
        return false;
    }

    if ((y < min(firstLine.y1, firstLine.y2)       - epsilon) ||
            (y > max(firstLine.y1, firstLine.y2)   + epsilon) ||
            (y < min(secondLine.y1, secondLine.y2) - epsilon) ||
            (y > max(secondLine.y1, secondLine.y2) + epsilon))
    {
        // It's not
        intersectPoint.x = 0.0;
        intersectPoint.y = 0.0;
        return false;
    }

    // Ok, we have a valid intersection point
    intersectPoint.x = x;
    intersectPoint.y = y;
    return true;
}


#endif
