/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file direction_func.h Different functions related to conversions between directions. */

#ifndef DIRECTION_FUNC_H
#define DIRECTION_FUNC_H

#include "direction_type.h"
#include "core/math_func.hpp"

/**
 * Checks if an integer value is a valid DiagDirection
 *
 * @param d The value to check
 * @return True if the value belongs to a DiagDirection, else false
 */
static inline bool IsValidDiagDirection(DiagDirection d)
{
	return d < DIAGDIR_END;
}

/**
 * Checks if an integer value is a valid Direction
 *
 * @param d The value to check
 * @return True if the value belongs to a Direction, else false
 */
static inline bool IsValidDirection(Direction d)
{
	return d < DIR_END;
}

/**
 * Checks if an integer value is a valid Axis
 *
 * @param d The value to check
 * @return True if the value belongs to an Axis, else false
 */
static inline bool IsValidAxis(Axis d)
{
	return d < AXIS_END;
}

/**
 * Return the reverse of a direction
 *
 * @param d The direction to get the reverse from
 * @return The reverse Direction
 */
static inline Direction ReverseDir(Direction d)
{
	assert(IsValidDirection(d));
	return (Direction)(4 ^ d);
}


/**
 * Calculate the difference between two directions
 *
 * @param d0 The first direction as the base
 * @param d1 The second direction as the offset from the base
 * @return The difference how the second direction drifts of the first one.
 */
static inline DirDiff DirDifference(Direction d0, Direction d1)
{
	assert(IsValidDirection(d0));
	assert(IsValidDirection(d1));
	/* Cast to uint so compiler can use bitmask. If the difference is negative
	 * and we used int instead of uint, further "+ 8" would have to be added. */
	return (DirDiff)((uint)(d0 - d1) % 8);
}

/**
 * Applies two differences together
 *
 * This function adds two differences together and returns the resulting
 * difference. So adding two DIRDIFF_REVERSE together results in the
 * DIRDIFF_SAME difference.
 *
 * @param d The first difference
 * @param delta The second difference to add on
 * @return The resulting difference
 */
static inline DirDiff ChangeDirDiff(DirDiff d, DirDiff delta)
{
	/* Cast to uint so compiler can use bitmask. Result can never be negative. */
	return (DirDiff)((uint)(d + delta) % 8);
}

/**
 * Change a direction by a given difference
 *
 * This functions returns a new direction of the given direction
 * which is rotated by the given difference.
 *
 * @param d The direction to get a new direction from
 * @param delta The offset/drift applied to the direction
 * @return The new direction
 */
static inline Direction ChangeDir(Direction d, DirDiff delta)
{
	assert(IsValidDirection(d));
	/* Cast to uint so compiler can use bitmask. Result can never be negative. */
	return (Direction)((uint)(d + delta) % 8);
}


/**
 * Returns the reverse direction of the given DiagDirection
 *
 * @param d The DiagDirection to get the reverse from
 * @return The reverse direction
 */
static inline DiagDirection ReverseDiagDir(DiagDirection d)
{
	assert(IsValidDiagDirection(d));
	return (DiagDirection)(2 ^ d);
}

/**
 * Calculate the difference between two DiagDirection values
 *
 * @param d0 The first direction as the base
 * @param d1 The second direction as the offset from the base
 * @return The difference how the second direction drifts of the first one.
 */
static inline DiagDirDiff DiagDirDifference(DiagDirection d0, DiagDirection d1)
{
	assert(IsValidDiagDirection(d0));
	assert(IsValidDiagDirection(d1));
	/* Cast to uint so compiler can use bitmask. Result can never be negative. */
	return (DiagDirDiff)((uint)(d0 - d1) % 4);
}

/**
 * Applies a difference on a DiagDirection
 *
 * This function applies a difference on a DiagDirection and returns
 * the new DiagDirection.
 *
 * @param d The DiagDirection
 * @param delta The difference to apply on
 * @return The new direction which was calculated
 */
static inline DiagDirection ChangeDiagDir(DiagDirection d, DiagDirDiff delta)
{
	assert(IsValidDiagDirection(d));
	/* Cast to uint so compiler can use bitmask. Result can never be negative. */
	return (DiagDirection)((uint)(d + delta) % 4);
}

/**
 * Convert a Direction to a DiagDirection.
 *
 * This function can be used to convert the 8-way Direction to
 * the 4-way DiagDirection. If the direction cannot be mapped its
 * "rounded clockwise". So DIR_N becomes DIAGDIR_NE.
 *
 * @param dir The direction to convert
 * @return The resulting DiagDirection, maybe "rounded clockwise".
 */
static inline DiagDirection DirToDiagDir(Direction dir)
{
	assert(IsValidDirection(dir));
	return (DiagDirection)(dir >> 1);
}

/**
 * Convert a DiagDirection to a Direction.
 *
 * This function can be used to convert the 4-way DiagDirection
 * to the 8-way Direction. As 4-way are less than 8-way not all
 * possible directions can be calculated.
 *
 * @param dir The direction to convert
 * @return The resulting Direction
 */
static inline Direction DiagDirToDir(DiagDirection dir)
{
	assert(IsValidDiagDirection(dir));
	return (Direction)(dir * 2 + 1);
}


/**
 * Select the other axis as provided.
 *
 * This is basically the not-operator for the axis.
 *
 * @param a The given axis
 * @return The other axis
 */
static inline Axis OtherAxis(Axis a)
{
	assert(IsValidAxis(a));
	return (Axis)(a ^ 1);
}


/**
 * Convert a DiagDirection to the axis.
 *
 * This function returns the axis which belongs to the given
 * DiagDirection. The axis X belongs to the DiagDirection
 * north-east and south-west.
 *
 * @param d The DiagDirection
 * @return The axis which belongs to the direction
 */
static inline Axis DiagDirToAxis(DiagDirection d)
{
	assert(IsValidDiagDirection(d));
	return (Axis)(d & 1);
}


/**
 * Converts an Axis to a DiagDirection
 *
 * This function returns the DiagDirection which
 * belongs to the axis. As 2 directions are mapped to an axis
 * this function returns the one which points to south,
 * either south-west (on X axis) or south-east (on Y axis)
 *
 * @param a The axis
 * @return The direction pointed to south
 */
static inline DiagDirection AxisToDiagDir(Axis a)
{
	assert(IsValidAxis(a));
	return (DiagDirection)(2 - a);
}

/**
 * Converts an Axis to a Direction
 *
 * This function returns the Direction which
 * belongs to the axis. As 2 directions are mapped to an axis
 * this function returns the one which points to south,
 * either south-west (on X axis) or south-east (on Y axis)
 *
 * @param a The axis
 * @return The direction pointed to south
 */
static inline Direction AxisToDirection(Axis a)
{
	assert(IsValidAxis(a));
	return (Direction)(5 - 2 * a);
}

/**
 * Convert an axis and a flag for north/south into a DiagDirection
 * @param xy axis to convert
 * @param ns north -> 0, south -> 1
 * @return the desired DiagDirection
 */
static inline DiagDirection XYNSToDiagDir(Axis xy, uint ns)
{
	assert(IsValidAxis(xy));
	return (DiagDirection)(xy * 3 ^ ns * 2);
}

/**
 * Checks if a given Direction is diagonal.
 *
 * @param dir The given direction.
 * @return True if the direction is diagonal.
 */
static inline bool IsDiagonalDirection(Direction dir)
{
	assert(IsValidDirection(dir));
	return (dir & 1) != 0;
}

/**
 * Checks if an integer value is a valid DirTransformation.
 *
 * @param transformation The value to check.
 * @return True if the value belongs to a DirTransformation, false otherwise.
 */
static inline bool IsValidDirTransform(DirTransformation transformation)
{
	return IsInsideMM(transformation, DTR_BEGIN, DTR_END);
}

/**
 * Combine two direction transformations into one.
 * @param a First transformation.
 * @param b Second transformation.
 * @return Transformation that works like firstly applying the 'a' transformation and then the 'b' transformation.
 */
static inline DirTransformation CombineDirTransform(DirTransformation a, DirTransformation b)
{
	/* DirTransformation is an enum representing a reflection+rotation transformstion
	 * and consists of following bits:
	 * (MSB->)  00000irr  (<-LSB)
	 * where:
	 *     00000 - unused bits, always 0
	 *     i - DTR_REFLECTION_BIT, whether to reflect in respect to X-axis before rotating
	 *     rr - DTR_ROTATION_MASK, how many 90-degree clockwise rotations to perform
	 *
	 * To help understand how to combine two DirTransformation values
	 * we can express the transformation as a function of an angle:
	 *   f(x) = I * x + R
	 * where
	 *   x - unified angle
	 *   I - reflection, -1 to reflect (DTR_REFLECTION_BIT bit set to 1),
	 *          +1 otherwise (DTR_REFLECTION_BIT bit set to 0)
	 *   R - rotation, how much to rotate (bits of mask DTR_ROTATION_MASK)
	 *
	 * As we work on angles we must use modular arithmetic when adding rotations together.
	 * Modulus is 4 because 360 degree (full circle) consist of 4 rotations (360 = 90 * 4).
	 * To apply the modulus we can simply bitmask the result with DTR_ROTATION_MASK bits.
	 *
	 * To combine two transformations A and B
	 *   a(x) = IA * x + RA
	 *   b(x) = IB * x + RB
	 * into single transformation C
	 *   c(x) = IC * x + RC
	 * we must compose the functions
	 *   c(x) = b(a(x)) = IB * (IA * x + RA) + RB
	 * which can be reduced to
	 *   c(x) = IA * IB * x + IB * RA + RB
	 *         |       |     |            |
	 *         |<-IC ->|     |<-   RC   ->|
	 * from above
	 *   IC = IA * IB
	 * thus to calculate reflection bit of C we can XOR reflection bits of A and B, and
	 *   RC = IB * RA + RB
	 * thus to calculate rotation bits of C we can add RB+RA or subtract RB-RA, based on the
	 * reflection bit of the B transformation. */
	return (DirTransformation)(((a ^ b) & DTR_REFLECTION_BIT) | // calculate reflection bit
		(((b & DTR_REFLECTION_BIT) ? (b - a) : (b + a)) & DTR_ROTATION_MASK)); // calculate rotation bits
}

/**
 * Invert given transformation.
 * @param transformation Tranformation to invert.
 * @return Inverted transformation transformation.
 */
static inline DirTransformation InvertDirTransform(DirTransformation transformation)
{
	/* to revert a reflection reflect again, transformation is the same (involution) */
	if (transformation & DTR_REFLECTION_BIT) return transformation;

	/* to revert a rotation rotate in opposite direction */
	return (DirTransformation)((-transformation) & DTR_ROTATION_MASK);
}

/**
 * Transform Axis by a given transformation.
 * @param axis Axis to transform.
 * @param transformation Transformation to use.
 * @return Transformed Axis.
 */
static inline Axis TransformAxis(Axis axis, DirTransformation transformation)
{
	return (Axis)(axis ^ (transformation & 1));
}

/**
 * Transform Direction by given transformation.
 * @param direction Direction to transform.
 * @param transformation Transformation to use.
 * @return Transformed Direction.
 */
static inline Direction TransformDir(Direction direction, DirTransformation transformation)
{
	if (transformation & DTR_REFLECTION_BIT) direction = (Direction)(2 * DIR_NE - direction); // reflect against X-axis
	return ChangeDir(direction, (DirDiff)(2 * transformation)); // rotate and cut off overflowing bits
}

/**
 * Transform DiagDirection by a given transformation.
 * @param diag_dir DiagDirection to transform.
 * @param transformation Transformation to use.
 * @return Transformed DiagDirection.
 */
static inline DiagDirection TransformDiagDir(DiagDirection diag_dir, DirTransformation transformation)
{
	if (transformation & DTR_REFLECTION_BIT) diag_dir = (DiagDirection)(2 * DIAGDIR_NE - diag_dir); // reflect against X-axis
	return ChangeDiagDir(diag_dir, (DiagDirDiff)transformation); // rotate and cut off overflowing bits
}

#endif /* DIRECTION_FUNC_H */
