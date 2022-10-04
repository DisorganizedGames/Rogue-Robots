
-- Simple linear algebra functionality for lua scripts
-- Prefer doing complex linear algebra in c++ (It's vector register accelerated there)

-- Basic vector structure to use in this libraries functions

Vector3 = {
	new = function(x, y, z)
		local vec = { x=x, y=y, z=z }
		setmetatable(vec, Vector3)
		return vec
	end,

	fromTable = function(vecTable)
		local vec = vecTable
		setmetatable(vec, Vector3)
		return vec
	end,

	-- Vector addition
	__add = function(first, second)
		return Vector3.new(first.x + second.x, first.y + second.y, first.z + second.z)
	end,
	
	-- Vector subtraction
	__sub = function(first, second)
		return Vector3.new(first.x - second.x, first.y - second.y, first.z - second.z)
	end,
	
	-- Vector inversion
	__unm = function(vec)
		return Vector3.new(-vec.x, -vec.y, -vec.z)
	end,

	-- Vector-scalar multiplication
	__mul = function(vec, scalar)
		return Vector3.new(scalar * vec.x, scalar * vec.y, scalar * vec.z)
	end
}

-- Returns the vector sum of vectors v and u
function VectorAdd(v, u)
	return v + u
end

-- Subtracts vector u from vector v
function VectorSub(v, u)
	return v - u
end

-- Scale vector v by scalar s
function Scale(v, s)
	return v * s
end

function QuatMul(q, p)
	local result = {
		x = ( (q.x*p.x) - (q.y*p.y) - (q.z*p.z) - (q.w*p.w) ),
		y = ( (q.x*p.y) + (q.y*p.x) + (q.z*p.w) - (q.w*p.z) ),
		z = ( (q.x*p.z) - (q.y*p.w) + (q.z*p.x) + (q.w*p.y) ),
		w = ( (q.x*p.w) + (q.y*p.z) - (q.z*p.y) + (q.w*p.x) ),
	}

	return result
end


-- Rotates vector v around axis by angle radians clockwise
-- Axis has to be a unit vector for correct rotations
function RotateAroundAxis(v, axis, angle)
	-- Construct rotation quaternion and its inverse
	local f = math.sin(angle / 2.0)
	local rotQuat = {
		x = math.cos(angle / 2.0),
		y = axis.x * f,
		z = axis.y * f,
		w = axis.z * f,
	}

	local rotQuatInv = {
		x = rotQuat.x,
		y = -rotQuat.y,
		z = -rotQuat.z,
		w = -rotQuat.w,
	}
	
	-- Multiply qpq^(-1) (where p is the quaternionized version of v, with real part 0)
	local quatV = {x = 0, y = v.x, z = v.y, w = v.z}

	local outV = QuatMul(rotQuatInv, QuatMul(quatV, rotQuat))

	return Vector3.new(outV.y, outV.z, outV.w)
end

