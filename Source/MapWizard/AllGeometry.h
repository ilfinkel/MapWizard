// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

struct Node;
struct Point;

class MAPWIZARD_API AllGeometry
{
public:
	static TOptional<FVector> is_intersect(const FVector& line1_begin,
	                                       const FVector& line1_end,
	                                       const FVector& line2_begin,
	                                       const FVector& line2_end,
	                                       bool is_opened);


	static TOptional<TTuple<FVector, TTuple<
		                        TSharedPtr<Node>, TSharedPtr<Node>>>>
	is_intersect_array(
		const TSharedPtr<Node>& line1_begin, const TSharedPtr<Node>& line1_end,
		const TArray<TSharedPtr<Node>>& lines,
		bool is_opened);
	static TOptional<TTuple<FVector, TTuple<
		                        TSharedPtr<Node>, TSharedPtr<Node>>>>
	is_intersect_array(
		FVector line1_begin, FVector line1_end,
		const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static TOptional<FVector> is_intersect_array(
		FVector line_begin, FVector line_end,
		const TArray<FVector>& array_point, bool is_opened);
	static TOptional<TSharedPtr<Node>> is_intersect_array_clear(
		const TSharedPtr<Node>& line1_begin,
		const TSharedPtr<Node>& line1_end,
		const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static int is_intersect_array_count(const TSharedPtr<Node>& line_begin,
	                                    const TSharedPtr<Node>& line_end,
	                                    const TArray<TSharedPtr<Node>>& lines,
	                                    bool is_opened);
	static TOptional<FVector> is_intersect_array_clear(
		const FVector& line_begin, const FVector& line_end,
		const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static FVector create_segment_at_angle(const FVector& line_begin,
	                                       const FVector& line_end,
	                                       const FVector& line_beginPoint,
	                                       double angle_in_degrees,
	                                       double length);
	static float calculate_angle(const FVector& A, const FVector& B,
	                             const FVector& C, bool is_clockwork = false);
	static float calculate_angle_clock(const FVector& A, const FVector& B,
	                                   const FVector& C,
	                                   bool is_clockwork = false);
	static float calculate_angle_counterclock(const FVector& A,
	                                          const FVector& B,
	                                          const FVector& C,
	                                          bool is_clockwork = false);
	static float get_poygon_area(const TArray<TSharedPtr<Node>>& Vertices);
	static float get_poygon_area(const TArray<TSharedPtr<Point>>& Vertices);
	static float get_poygon_area(const TArray<Point>& Vertices);
	static bool IsConvex(const FVector& Prev, const FVector& Curr,
	                     const FVector& Next);
	static bool IsEar(TArray<FVector> Vertices, int32 PrevIndex,
	                  int32 CurrIndex, int32 NextIndex,
	                  TArray<int32> RemainingVertices);
	static bool IsPointInTriangle(const FVector& Point, const FVector& A,
	                              const FVector& B, const FVector& C);

	static void TriangulatePolygon(const TArray<FVector>& Polygon,
	                               TArray<int32>& Triangles);
	static bool is_point_in_figure(FVector point_, TArray<FVector> figure);
	static float point_to_seg_distance(const FVector& SegmentStart,
	                                   const FVector& SegmentEnd,
	                                   const FVector& Point);
	static FVector point_to_seg_distance_get_closest(const FVector& SegmentStart,
	                                                 const FVector& SegmentEnd,
	                                                 const FVector& Point);
	static bool is_point_near_figure(const TArray<FVector> given_line,
	                                 const FVector& Point, double distance);
	static TArray<FVector> line_to_polygon(const TArray<FVector> given_line,
	                                       double width);
	static TArray<FVector> shrink_polygon(const TArray<FVector> cur_polygon, double interval);
};


enum class point_type
{
	unidentified = 0,
	road = 1,
	main_road = 2,
	wall = 3,
	river = 4,
};

enum class district_type
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

static unsigned int point_counter = 0;

struct Point
{
	Point() : point(FVector(0, 0, 0))
	          , type()
	{
		point_id = point_counter;
		point_counter++;
	}

	Point(double X, double Y, double Z) : point(FVector(X, Y, Z))
	                                      , type()
	{
		point_id = point_counter;
		point_counter++;
	}

	Point(FVector node_) : Point(node_.X, node_.Y, node_.Z)

	{
		point_id = point_counter;
		point_counter++;
	}

	~Point();
	FVector point;
	point_type type;
	bool used = false;
	TArray<district_type> districts_nearby;
	unsigned int point_id = 0;

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

	bool operator==(const Point& Other) const
	{
		return this->point_id == Other.point_id;
	}
};

enum class object_type
{
	house,
	street,
	district,
	unknown
};

static unsigned int object_counter = 0;

struct SelectableObject
{
	SelectableObject()
	{
		object_counter++;
		object_id = object_counter;
	}

	unsigned int get_id() const { return object_id; }
	void select() { selected = true; }
	void unselect() { selected = false; }
	void hover() { hovered = true; }
	void unhover() { hovered = false; }

	void set_angle(float angle_)
	{
		angle = angle_;
	}

	virtual float get_angle()
	{
		return angle;
	}

	virtual TArray<FVector> get_object_vertexes()
	{
		return {};
	}

	virtual TArray<TArray<FVector>> slice_house(float north, float east, float south, float west)
	{
		TArray<TArray<FVector>> house_slices;
		house_slices.Reserve(9);
		return house_slices;
	}

	virtual FVector get_measure() { return FVector(0, 0, 0); }
	FString get_object_type() { return object_type; }
	bool is_selected() { return selected; };
	bool is_hovered() { return hovered; };
	bool operator==(SelectableObject& other) const { return this->object_id == other.object_id; }

protected:
	bool selected = false;
	bool hovered = false;
	FString object_type;
	unsigned int object_id;
	float angle = 0;
};

struct Lighter : public SelectableObject
{
	Lighter(FVector position_, double intensity_)
	{
		object_type = "Lighter";
		position = position_;
		intensity = intensity_;
	};
	bool operator==(Lighter& other) const { return FVector::DistSquared(this->position, other.position) < 0.1; }

	TArray<FVector> get_object_vertexes() override
	{
		return {position};
	}

	FVector position;
	double intensity;
	// private:
};

struct Street : public SelectableObject
{
	Street()
	{
		object_type = "Street";
	};

	Street(TArray<TSharedPtr<Node>> points_): street_vertices(points_)
	{
		object_type = "Street";
	}

	// float get_angle() override
	// {
	// 	return angle;
	// }

	FVector get_measure() override
	{
		return FVector();
	}

	TArray<FVector> get_object_vertexes() override
	{
		return street_vertexes;
	}

	TArray<TSharedPtr<Node>> get_street_vertices()
	{
		return street_vertices;
	}

	TArray<FVector> street_vertexes;
	TArray<TSharedPtr<Node>> street_vertices;
	point_type type = point_type::unidentified;
	FString name;
};

struct House : public SelectableObject
{
	House(TArray<FVector> figure_, double height_) : house_figure(figure_)
	                                                 , height(height_)
	{
		object_type = "House";
	}

	House(TArray<FVector> figure_, double height_, FString obj_type) : house_figure(figure_)
	                                                                   , height(height_)
	{
		object_type = obj_type;
	}

	~House();

	float get_angle() override
	{
		FVector f1 = house_figure[0];
		FVector f2 = house_figure[1];
		FVector f3 = house_figure[1];
		f3.X += 1000;
		angle = AllGeometry::calculate_angle_clock(f1, f2, f3);
		set_angle(angle);
		return angle;
	}

	FVector get_measure() override
	{
		FVector vec;
		vec.X = FVector::Distance(house_figure[0], house_figure[1]);
		vec.Y = FVector::Distance(house_figure[1], house_figure[2]);
		vec.Z = height;
		return vec;
	}

	TArray<FVector> get_object_vertexes() override
	{
		return house_figure;
	}

	TArray<TArray<FVector>> slice_house(float north, float east, float south, float west);

	TArray<FVector> house_figure;
	double height;
};


struct District : public SelectableObject
{
	explicit District(): main_roads(0)
	                     , is_river_in(false)
	{
		// UE_LOG(LogTemp, Warning, TEXT("ditrict(%p)"),this)
		type = district_type::unknown;
		area = 0;
		figure = TArray<TSharedPtr<Node>>();

		object_type = "District";
	}

	explicit District(TArray<TSharedPtr<Node>> figure_);

	~District()
	{
		// UE_LOG(LogTemp, Warning, TEXT("~ditrict"))
		figure.Empty();
		self_figure.Empty();
	}

	void clear_me()
	{
		figure.Empty();
		self_figure.Empty();
	}

	// float get_angle() override
	// {
	// 	return angle;
	// }

	FVector get_measure() override
	{
		return FVector();
	}

	TArray<FVector> get_object_vertexes() override
	{
		TArray<FVector> vertexes;
		for (auto vertex : self_figure)
		{
			vertexes.Add(vertex.point);
		}
		return vertexes;
	}

	TArray<TSharedPtr<Node>> figure;
	TArray<Point> self_figure;
	TArray<TSharedPtr<House>> houses;
	double area;
	int main_roads;
	bool is_river_in;
	void set_district_type(district_type type_);
	district_type get_district_type() { return type; }
	bool is_point_in_self_figure(FVector point_);
	bool is_point_in_figure(FVector point_);
	void get_self_figure();
	TArray<Point> shrink_figure_with_roads(
		TArray<TSharedPtr<Node>>& figure_vertices, float road, float main_road);
	TOptional<FVector> is_line_intersect(FVector point1, FVector point2);
	bool create_house(TArray<FVector> given_line, double width, double height, FString obj_type);
	bool create_house(TArray<FVector> given_line, double height, FString obj_type, bool trustworthy);
	bool attach_district(TSharedPtr<District> other_district,
	                     TArray<TSharedPtr<Street>>& streets_to_delete);
	bool divide_me(TSharedPtr<District> dist1, TSharedPtr<District> dist2,
	               TSharedPtr<Street> new_seg);
	bool is_adjacent(TSharedPtr<District> other_district);

private:
	district_type type;
};

struct Conn
{
	Conn(TSharedPtr<Node> node_,
	     TSharedPtr<TArray<TSharedPtr<Node>>> figure_) : node(node_)
	                                                     , figure(figure_)
	{
		not_in_figure = false;
		in_street = false;
	}

	Conn(TSharedPtr<Node> node_) : node(node_)
	{
		figure = MakeShared<TArray<TSharedPtr<Node>>>();
		not_in_figure = false;
		in_street = false;
	}

	~Conn();
	void set_street(TSharedPtr<Street> street_) { street = street_; }
	void set_segment(TSharedPtr<Street> segment_) { segment = segment_; }
	TSharedPtr<Street> get_street() { return segment; }
	TSharedPtr<Node> node = nullptr;

	// point_type street_type = point_type::road;
	TSharedPtr<TArray<TSharedPtr<Node>>> figure{};
	// TSharedPtr<TArray<TSharedPtr<Point>>> street{};
	point_type street_type() { return segment.IsValid() ? segment->type : point_type::road; }

	bool not_in_figure;
	bool in_street;
	bool operator==(Conn& other) { return this->node == other.node; }

private:
	TSharedPtr<Street> street;
	TSharedPtr<Street> segment;
};

struct Node : TSharedFromThis<Node>
{
	Node(double X, double Y, double Z, int debug_ind = 0) :
		point(MakeShared<Point>(FVector(X, Y, Z)))
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
	point_type get_node_type() { return point->type; }
	void set_type(point_type type_) { point->type = type_; }
	TOptional<TSharedPtr<Conn>> get_next_point(TSharedPtr<Point> point_);
	TOptional<TSharedPtr<Conn>> get_prev_point(TSharedPtr<Point> point_);
	void add_connection(const TSharedPtr<Node>& node_);

	void delete_me();

	bool operator==(const Node& other) const
	{
		return this->point->point_id == other.point->point_id;
	}

	bool operator==(const Point& other) const
	{
		return this->point->point_id == other.point_id;
	}

	void print_connections();
	void set_unshrinkable() { unshrinkable = true; }

protected:
	TSharedPtr<Point> point;

public:
	int debug_ind_ = 0;
	bool unshrinkable;
	bool in_figure;
	bool unmovable = false;
};
