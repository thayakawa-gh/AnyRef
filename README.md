# AnyRef
Generic reference to any object to enable generics-like dynamic polymorphism by non-function templates.

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

#### 2. dynamic generics
```cpp
struct Visitor1_0
{
	using ArgTypes = std::tuple<>;
	using RetType = void;
	template <class S, class T, class U>
	void operator()(const S& a, const T& b, U& res) const
	{
		res = a + b;
	}
};
void FuncAnyRefGenerics2(Generics<std::tuple<AnyCRef, AnyCRef, AnyRef>, std::tuple<Visitor1_0>> a)
{
	a.Visit<0>();
}
void ExampleAnyRefGenerics2()
{
	int ires;
	FuncAnyRefGenerics2(std::forward_as_tuple(1, 2, ires));
	std::cout << "result of Visit1_1 with int == " << ires << std::endl;
	std::string sres;
	FuncAnyRefGenerics2(std::forward_as_tuple(std::string("123"), "456", sres));
	std::cout << "result of Visit1_1 with std::string == " << sres << std::endl;
}
/*--output--
result of Visit1_1 with int == 3
result of Visit1_1 with std::string == 123456
*/
```
