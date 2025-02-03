// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

struct Node;
class AllGeometry;
enum point_type
{
	unidentified = 0,
	road = 1,
	main_road = 2,
	wall = 3,
	river = 4,
};

enum district_type
{
	water,
	royal,
	dock,
	luxury,
	residential,
	slums,
	tower,
	empty,
	unknown
};


struct WeightedPoint
{
	WeightedPoint(const FVector& point_, const double weight_) : point(point_)
	                                                           , weight(weight_)
	{
	}
	FVector point;
	double weight;
};


struct Point
{
	Point() : point(FVector(0, 0, 0))
	        , type()
	{
	}
	Point(double X, double Y, double Z) : point(FVector(X, Y, Z))
	                                    , type()
	{
	}

	Point(FVector node_) : Point(node_.X, node_.Y, node_.Z)
	                                             
	{
	}
	~Point();
	FVector point;
	point_type type;
	bool used = false;
	TArray<district_type> districts_nearby;

	Point& operator=(const Point& Other)
	{
		if (this != &Other)
		{
			point = Other.point;
			type = Other.type;
			used = Other.used;
			districts_nearby = Other.districts_nearby;
		}
		return *this;
	}

};

struct Street
{
	Street(TArray<TSharedPtr<Node>> points_): street_vertices(points_)
	                                        , type(unidentified)
	{
	}
	TArray<FVector> street_vertexes;
	TArray<TSharedPtr<Node>> street_vertices;
	point_type type;
	FString name;
	
};

// struct Way
// {
// 	Way(): type()
// 	{
// 	}
//
// 	Way(TArray<TSharedPtr<Point>> points_) : points(points_)
// 	                                       , type()
// 	{
// 	}
// 	~Way() { points.Empty(); }
// 	TArray<TSharedPtr<Point>> points;
// 	point_type type;
// 	FString name;
// };

struct Conn
{
	Conn(TSharedPtr<Node> node_, TSharedPtr<TArray<TSharedPtr<Node>>> figure_) : node(node_)
	                                                                           , figure(figure_)
	{
		not_in_figure = false;
		in_street = false;
	}

	Conn(TSharedPtr<Node> node_) : node(node_)
	{
		figure = MakeShared<TArray<TSharedPtr<Node>>>();
		// street = MakeShared<TArray<TSharedPtr<Point>>>();
		not_in_figure = false;
		in_street = false;
		street_type = road;
	}
	~Conn();
	TSharedPtr<Node> node;
	point_type street_type = road;
	TSharedPtr<TArray<TSharedPtr<Node>>> figure{};
	// TSharedPtr<TArray<TSharedPtr<Point>>> street{};
	bool not_in_figure;
	bool in_street;
	bool operator==(Conn& other) { return this->node == other.node; }
};

struct Node : TSharedFromThis<Node>
{
	Node(double X, double Y, double Z, int debug_ind = 0) : point(MakeShared<Point>(FVector(X, Y, Z)))
	                                                      , debug_ind_(debug_ind)
	                                                      , unshrinkable(false)
	                                                      , in_figure(false)
	{
	}
	Node() : point(MakeShared<Point>(FVector(0, 0, 0)))
	       , unshrinkable(false)
	       , in_figure(false)
	{
	}
	Node(FVector node_) : point(MakeShared<Point>(node_.X, node_.Y, node_.Z))
	                    , unshrinkable(false)
	                    , in_figure(false)
	{
	}
	~Node()
	{
		conn.Empty();
		point.Reset();
	}
	TArray<TSharedPtr<Conn>> conn;
	void set_FVector(FVector point_) { point->point = point_; }
	void set_FVector_X(double X) { point->point.X = X; }
	void set_FVector_Y(double Y) { point->point.Y = Y; }
	void set_FVector_Z(double Z) { point->point.Z = Z; }
	FVector get_FVector() { return point->point; }
	TSharedPtr<Point> get_point() { return point; }
	bool is_used() { return point->used; }
	void set_used() { point->used = true; }
	void set_used(bool used_) { point->used = used_; }
	void set_unmovable() { unmovable = true; }
	bool is_unmovable() { return unmovable; }
	point_type get_type() { return point->type; }
	void set_type(point_type type_) { point->type = type_; }
	TOptional<TSharedPtr<Conn>> get_next_point(TSharedPtr<Point> point_);
	TOptional<TSharedPtr<Conn>> get_prev_point(TSharedPtr<Point> point_);
	void add_connection(const TSharedPtr<Node>& node_);
	void delete_me();
	bool operator==(const Node&) const { return FVector::Distance(this->point->point, point->point) < 0.001; }
	void print_connections();

protected:
	TSharedPtr<Point> point;

public:
	int debug_ind_ = 0;
	bool unshrinkable;
	bool in_figure;
	bool unmovable = false;
};

struct House
{
	House(TArray<FVector> figure_, double height_) : house_figure(figure_)
	                                               , height(height_)
	{
	}
	~House();
	TArray<FVector> house_figure;
	double height;
};



struct District
{
	District(): main_roads(0)
	          , is_river_in(false)
	{
		type = district_type::unknown;
		area = 0;
		figure = TArray<TSharedPtr<Node>>();
	}

	District(TArray<TSharedPtr<Node>> figure_);
	~District()
	{
		figure.Empty();
		self_figure.Empty();
	}
	TArray<TSharedPtr<Node>> figure;
	TArray<Point> self_figure;
	TArray<House> houses;
	double area;
	int main_roads;
	bool is_river_in;
	void set_type(district_type type_);
	district_type get_type() { return type; }
	bool is_point_in_self_figure(FVector point_);
	bool is_point_in_figure(FVector point_);
	void get_self_figure();
	TArray<Point> shrink_figure_with_roads(TArray<TSharedPtr<Node>>& figure_vertices, float road, float main_road);
	TOptional<FVector> is_line_intersect(FVector point1, FVector point2);
	bool create_house(TArray<FVector> given_line, double width, double height);

private:
	district_type type;
};


class MAPWIZARD_API AllGeometry
{
public:
	static TOptional<FVector> is_intersect(const FVector& line1_begin, const FVector& line1_end,
	                                       const FVector& line2_begin, const FVector& line2_end, bool is_opened);


	static TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>> is_intersect_array(
		const TSharedPtr<Node>& line1_begin, const TSharedPtr<Node>& line1_end, const TArray<TSharedPtr<Node>>& lines,
		bool is_opened);
	static TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>> is_intersect_array(
		FVector line1_begin, FVector line1_end, const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static TOptional<FVector> is_intersect_array(FVector line_begin, FVector line_end,
	                                             const TArray<FVector>& array_point, bool is_opened);
	static TOptional<TSharedPtr<Node>> is_intersect_array_clear(const TSharedPtr<Node>& line1_begin,
	                                                            const TSharedPtr<Node>& line1_end,
	                                                            const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static int is_intersect_array_count(const TSharedPtr<Node>& line_begin, const TSharedPtr<Node>& line_end,
	                                    const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static TOptional<FVector> is_intersect_array_clear(const FVector& line_begin, const FVector& line_end,
	                                                   const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static FVector create_segment_at_angle(const FVector& line_begin, const FVector& line_end,
	                                       const FVector& line_beginPoint, double angle_in_degrees, double length);
	static float calculate_angle(const FVector& A, const FVector& B, const FVector& C, bool is_clockwork = false);
	static float calculate_angle_clock(const FVector& A, const FVector& B, const FVector& C, bool is_clockwork = false);
	static float calculate_angle_counterclock(const FVector& A, const FVector& B, const FVector& C,
	                                          bool is_clockwork = false);
	static float get_poygon_area(const TArray<TSharedPtr<Node>>& Vertices);
	static float get_poygon_area(const TArray<TSharedPtr<Point>>& Vertices);
	static float get_poygon_area(const TArray<Point>& Vertices);
	static bool IsConvex(const FVector& Prev, const FVector& Curr, const FVector& Next);
	static bool IsEar(TArray<FVector> Vertices, int32 PrevIndex, int32 CurrIndex, int32 NextIndex,
	                  TArray<int32> RemainingVertices);
	static bool IsPointInTriangle(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);

	static void TriangulatePolygon(const TArray<FVector>& Polygon, TArray<int32>& Triangles);
	static bool is_point_in_figure(FVector point_, TArray<FVector> figure);
	static float point_to_seg_distance(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& Point);
	static bool is_point_near_figure(const TArray<FVector> given_line, const FVector& Point, double distance);
	static TArray<FVector> line_to_polygon(const TArray<FVector> given_line, double width, double height);
};
