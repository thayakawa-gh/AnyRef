# AnyRef
Generic reference to any object to allow template-like runtime polymorphism with non-function templates.

## Example
#### 1. AnyCRef ... generic const reference
```cpp
void FuncAnyCRef(AnyCRef a)
{
	if (a.Is<int>()) std::cout << "a is int " << a.Get<int>() << std::endl;
	else if (a.Is<double>()) std::cout << "a is double " << a.Get<double>() << std::endl;
	else if (a.Is<std::string>()) std::cout << "a is std::string " << a.Get<std::string>() << std::endl;
	else std::cout << "a is " << a.GetTypeIndex().name() << std::endl;
}
void ExampleAnyCRef()
{
	FuncAnyCRef(1);
	FuncAnyCRef(2.);
	FuncAnyCRef(std::string("3"));
	FuncAnyCRef(4.f);
}

/*--output--
a is int 1
a is double 2
a is std::string 3
a is float
*/
```

#### 2. run-time generics
```cpp
struct Addable
{
	using ArgTypes = std::tuple<>;
	using RetType = void;
	template <class S, class T, class U>
	void operator()(const S& a, const T& b, U& res) const
	{
		res = a + b;
	}
};
void FuncAnyRefGenerics2(Generics<std::tuple<AnyCRef, AnyCRef, AnyRef>, Addable> a)
{
	a.Visit<0>();
}
void ExampleAnyRefGenerics2()
{
	int ires;
	FuncAnyRefGenerics2(std::forward_as_tuple(1, 2, ires));
	std::cout << "result of Addable with int == " << ires << std::endl;
	std::string sres;
	FuncAnyRefGenerics2(std::forward_as_tuple(std::string("123"), "456", sres));
	std::cout << "result of Addable with std::string == " << sres << std::endl;
}
/*--output--
result of Addable with int == 3
result of Addable with std::string == 123456
*/
```

#### 3. run-time variadic function
```cpp
struct Accumulable
{
	using ArgTypes = std::tuple<std::ostream&>;
	using RetType = void;
	template <class ...T>
	void operator()(std::ostream& o, T&& ...v) const
	{
		o << "result of Accumulable::operator() with " << sizeof...(T) << " args = " << (... + v) << std::endl;
	}
};
struct RuntimeVariadicGenericsBase
{
	virtual ~RuntimeVariadicGenericsBase() = default;
	virtual void accumulate(Generics<Variadic<AnyURef>, Accumulable> a) const = 0;
};
struct RuntimeVariadicGenericsDerived : public RuntimeVariadicGenericsBase
{
	virtual void accumulate(Generics<Variadic<AnyURef>, Accumulable> a) const
	{
		a.Visit<0>(std::cout);
	}
};
void ExampleRuntimeVariadicGenerics()
{
	RuntimeVariadicGenericsBase* b = new RuntimeVariadicGenericsDerived();
	b->accumulate(std::forward_as_tuple(std::string("abc"), "def", "ghi"));
	b->accumulate(std::forward_as_tuple(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
	delete b;
}
/*--output--
result of Accumulable::operator() with 3 args = abcdefghi
result of Accumulable::operator() with 10 args = 55
*/
```
