#pragma once

template<class T>
struct Nullable
{
private:
	T value;
	bool hasValue;

public:
	Nullable() { hasValue = false; }
	Nullable(T value) : value(value) { hasValue = true; }
	bool HasValue() { return hasValue; }

	T GetValue() { return value; }
	void SetValue(T newValue) { value = newValue; hasValue = true; }
	void Clear() { hasValue = false; }
};