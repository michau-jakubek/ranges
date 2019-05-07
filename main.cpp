#include "view.hpp"

using os = std::ostream;
auto& oo = std::cout;
os& (*nn)(os&) = std::endl;

using namespace view;

struct point
{
	int x,y,z;
	point() : x(640), y(480), z(32) {}
	point(int x_, int y_) : x(x_), y(y_), z(32) {}
	point(int x_, int y_, int z_) : x(x_), y(y_) , z(z_) {}
	friend os& operator<<(os& str, const point& p)
	{
		return str << "{ " << p.x << ", " << p.y << ", " << p.z << " }";
	}
};

void test_where()
{
	std::vector<int> v { 1,2,3,4,5 };
	auto condition = [](auto&& a){ return 0 == (a % 2); };
	auto x = v | where(condition) | where([](auto&& a){return a > 3;});
}


void test_selecting()
{
	std::vector<int> v { 1,2,3,4,5 };
	auto x = v | selecting(selector<point>(), splaceholder(), 100);
	for (auto&& p : x)
	{
		oo << p << nn;
	}
}

void test_where_and_select()
{
	std::vector<int> source(10);
	std::iota(source.begin(), source.end(), 1);

	auto even = [](auto&& a){ return 0 == (a % 2); };
	auto pwr2 = [](const int& x) { return x*x; };

	auto v = source | where(even) | selecting(selector<point>(), splaceholder(), splaceholder(pwr2), 256);
	for (auto&& p : v)
	{
		oo << p << nn;
	}
}

int main()
{
	test_where_and_select();
	return 0;
}

