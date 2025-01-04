// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct Node;

enum point_type
{
	main,
	main_road,
	road,
	river,
	wall
};

enum district_type
{
	royal,
	dock,
	luxury,
	residential,
	slums,
	empty,
	unknown
};

struct WeightedPoint
{
	WeightedPoint(const FVector& point_, const double weight_) : point(point_)
	                                                           , weight(weight_)
	{
	};
	FVector point;
	double weight;
};

struct Point
{
	Point(double X, double Y, double Z) : point(FVector(X, Y, Z))
	{
	};

	Point() : point(FVector(0, 0, 0))
	{
	};

	Point(FVector node_) : Point(node_.X, node_.Y, node_.Z)
	{
	};
	~Point() { districts_nearby.Empty(); }
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

struct Conn
{
	Conn(TSharedPtr<Node> node_, TSharedPtr<TArray<TSharedPtr<Point>>> figure_) : node(node_)
	                                                                            , figure(figure_)
	{
		not_in_figure = false;
	}

	Conn(TSharedPtr<Node> node_) : node(node_)
	{
		figure = MakeShared<TArray<TSharedPtr<Point>>>();
		not_in_figure = false;
	}
	~Conn()
	{
		figure->Empty();
		node.Reset();
	}
	TSharedPtr<Node> node;
	TSharedPtr<TArray<TSharedPtr<Point>>> figure;
	bool not_in_figure;
	bool operator==(Conn& other) { return this->node == other.node; }
};

struct Node
{
	Node(double X, double Y, double Z, int debug_ind = 0) : point(MakeShared<Point>(FVector(X, Y, Z)))
	                                                      , debug_ind_(debug_ind)
	{
	};

	Node() : point(MakeShared<Point>(FVector(0, 0, 0)))
	{
	};

	Node(FVector node_) : point(MakeShared<Point>(node_.X, node_.Y, node_.Z))
	{
	};
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
	point_type get_type() { return point->type; }
	void set_type(point_type type_) { point->type = type_; }
	TOptional<TSharedPtr<Conn>> get_next_point(TSharedPtr<Point> point_);
	TOptional<TSharedPtr<Conn>> get_prev_point(TSharedPtr<Point> point_);
	void add_connection(const TSharedPtr<Node>& node_);
	void delete_me();
	bool operator==(const Node&) const { return this->point->point == point->point; }
	void print_connections();

protected:
	TSharedPtr<Point> point;

public:
	int debug_ind_;
	bool unshrinkable;
};

struct House
{
	House(TArray<FVector> figure_, double height_) : house_figure(figure_)
	                                               , height(height_)
	{
	};
	~House() { house_figure.Empty(); }
	TArray<FVector> house_figure;
	double height;
};

struct District
{
	District()
	{
		type = district_type::unknown;
		area = 0;
		figure = TArray<TSharedPtr<Point>>();
	};
	~District()
	{
		figure.Empty();
		self_figure.Empty();
	};
	District(TArray<TSharedPtr<Point>> figure_);
	TArray<TSharedPtr<Point>> figure;
	TArray<Point> self_figure;
	TArray<House> houses;
	double area;
	int main_roads;
	bool is_river_in;
	void set_type(district_type type_);
	district_type get_type() { return type; };
	bool is_point_in_self_figure(FVector point_);
	bool is_point_in_figure(FVector point_);
	void get_self_figure();
	bool shrink_size(TArray<Point>& Vertices, float road, float main_road);
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
	static float get_poygon_area(const TArray<TSharedPtr<Point>>& Vertices);
	static float get_poygon_area(const TArray<Point>& Vertices);
	static bool IsConvex(const FVector& Prev, const FVector& Curr, const FVector& Next);
	static bool IsEar(TArray<FVector> Vertices, int32 PrevIndex, int32 CurrIndex, int32 NextIndex,
	                  TArray<int32> RemainingVertices);
	static bool IsPointInTriangle(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);

	static void TriangulatePolygon(const TArray<FVector>& Polygon, TArray<int32>& Triangles);
	static bool is_point_in_figure(FVector point_, TArray<FVector> figure);
	static float point_to_seg_distance(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& Point);
};
