// Fill out your copyright notice in the Description page of Project Settings.


#include "AllGeometry.h"


District::District(TArray<TSharedPtr<Node>> figure_)
{
	// UE_LOG(LogTemp, Warning, TEXT("ditrict_figure(%p)"),this)
	// figure = figure_;
	bool is_found;
	if (figure_.Num() > 3)
	{
		do
		{
			int beg_del = 0;
			int end_del = 0;
			is_found = false;
			for (int i = 0; i < figure_.Num(); i++)
			{
				for (int j = i + 2; j < figure_.Num(); j++)
				{
					if (figure_[i] == figure_[j] && figure_[i + 1] == figure_[j
						- 1])
					{
						beg_del = i;
						end_del = j;
						is_found = true;
						break;
					}
				}
				if (is_found)
				{
					break;
				}
			}
			figure_.RemoveAt(beg_del, end_del - beg_del);
		}
		while (is_found);
	}
	figure = figure_;

	area = AllGeometry::get_poygon_area(figure);
	type = district_type::unknown;
	// if (area < 50000)
	// {
	// 	set_type(block_type::empty);
	// }

	get_self_figure();
}

void District::set_district_type(district_type type_)
{
	type = type_;
	for (int i = 0; i < figure.Num() - 2; i++)
	{
		figure[i]->get_point()->districts_nearby.Add(type_);
	}
}

bool District::is_point_in_self_figure(FVector point_)
{
	TArray<FVector> figure_to_check;
	for (auto& a : self_figure)
	{
		figure_to_check.Add(a.point);
	}
	return AllGeometry::is_point_in_figure(point_, figure_to_check);
}

bool District::is_point_in_figure(FVector point_)
{
	TArray<FVector> figure_to_check;
	for (auto& a : figure)
	{
		figure_to_check.Add(a->get_FVector());
	}
	return AllGeometry::is_point_in_figure(point_, figure_to_check);
}

void District::get_self_figure()
{
	self_figure.Empty();
	for (auto fig : figure)
	{
		if (fig.IsValid())
		{
			Point p = fig->get_FVector();
			self_figure.Add(p);
		}
	}
}

TArray<Point> District::shrink_figure_with_roads(
	TArray<TSharedPtr<Node>>& figure_vertices, float road, float main_road)
{
	int32 NumVertices = figure_vertices.Num();
	TArray<Point> new_points;
	auto backup_vertices = figure_vertices;
	if (figure_vertices[0]->get_point() == figure_vertices[NumVertices - 1]->
		get_point())
	{
		NumVertices--;
	}
	if (NumVertices < 3 || road / 2 <= 0.0f)
	{
		return TArray<Point>();
	}
	TArray<Point> exit_vertices;
	for (auto vertice : figure_vertices)
	{
		exit_vertices.Add(*vertice->get_point());
	}
	if (type == district_type::water)
	{
		return exit_vertices;
	}
	TArray<FVector> new_vertices;
	for (int i = 0; i <= NumVertices; ++i)
	{
		auto Prev = (i + NumVertices - 1) % NumVertices;
		auto Curr = i % NumVertices;
		auto Next = (i + 1) % NumVertices;
		float road_height1 = main_road / 2;
		float road_height2 = main_road / 2;
		auto prev_curr = figure_vertices[Prev]->get_next_point(
			figure_vertices[Curr]->get_point());
		auto curr_next = figure_vertices[Curr]->get_next_point(
			figure_vertices[Next]->get_point());
		if (prev_curr.IsSet())
		{
			if (prev_curr->IsValid())
			{
				if (prev_curr.GetValue()->street_type() == point_type::road)
				{
					road_height1 = road / 2;
				}
				else if (prev_curr.GetValue()->street_type() ==
					point_type::main_road)
				{
					road_height1 = main_road / 2;
				}
			}
			else road_height1 = 0;
		}
		else road_height1 = 0;
		if (curr_next.IsSet())
		{
			if (curr_next->IsValid())
			{
				if (curr_next.GetValue()->street_type() == point_type::road)
				{
					road_height2 = road / 2;
				}
				else if (curr_next.GetValue()->street_type() ==
					point_type::main_road)
				{
					road_height2 = main_road / 2;
				}
			}
			else road_height2 = 0;
		}
		else road_height2 = 0;


		FVector parralel1_beg = AllGeometry::create_segment_at_angle(
			figure_vertices[Prev]->get_FVector(),
			figure_vertices[Curr]->get_FVector(),
			figure_vertices[Prev]->get_FVector(), 90, road_height1);
		FVector parralel2_beg = AllGeometry::create_segment_at_angle(
			figure_vertices[Curr]->get_FVector(),
			figure_vertices[Next]->get_FVector(),
			figure_vertices[Next]->get_FVector(), 90, road_height2);
		FVector parralel1_end =
			AllGeometry::create_segment_at_angle(
				figure_vertices[Prev]->get_FVector(),
				figure_vertices[Curr]->get_FVector(), parralel1_beg, 0, 5000);
		FVector parralel2_end =
			AllGeometry::create_segment_at_angle(
				figure_vertices[Next]->get_FVector(),
				figure_vertices[Curr]->get_FVector(), parralel2_beg, 0, 5000);
		parralel1_beg.Z = 0;
		parralel2_beg.Z = 0;
		parralel1_end.Z = 0;
		parralel2_end.Z = 0;
		auto intersection =
			AllGeometry::is_intersect(parralel1_beg, parralel1_end,
			                          parralel2_beg, parralel2_end, false);

		if (intersection.IsSet())
		{
			if (!is_point_in_figure(intersection.GetValue()))
			{
				continue;
			}
			auto angle1 =
				AllGeometry::calculate_angle_clock(
					figure_vertices[Prev]->get_FVector(),
					figure_vertices[Curr]->get_FVector(),
					figure_vertices[Next]->get_FVector());
			auto angle2 = AllGeometry::calculate_angle_clock(
				parralel1_beg, intersection.GetValue(), parralel2_beg);
			if (angle2 - angle1 < 0.1)
			{
				bool is_valid = true;
				for (int j = 1; j <= figure.Num(); j++)
				{
					if (AllGeometry::point_to_seg_distance(
						figure_vertices[j - 1]->get_FVector(),
						figure_vertices[j % figure.Num()]->get_FVector(),
						intersection.GetValue()) < road / 2)
					{
						is_valid = false;
						break;
					}
				}
				if (is_valid == true)
				{
					Point new_point = *figure_vertices[Curr]->get_point();
					new_point.point = intersection.GetValue();
					new_points.Add(new_point);
				}
			}
		}
	}

	area = AllGeometry::get_poygon_area(new_points);
	return new_points;
}

TOptional<FVector> District::is_line_intersect(FVector point1, FVector point2)
{
	int NumVertices = self_figure.Num();
	for (int i = 1; i <= NumVertices; i++)
	{
		TOptional<FVector> intersect = AllGeometry::is_intersect(
			point1, point2, self_figure[i - 1].point,
			self_figure[i % NumVertices].point, false);
		if (intersect.IsSet())
		{
			return intersect.GetValue();
		}
	}
	for (int i = 0; i < houses.Num(); i++)
	{
		int house_figure_num = houses[i]->house_figure.Num();
		for (int j = 1; j <= house_figure_num; j++)
		{
			TOptional<FVector> intersect = AllGeometry::is_intersect(
				point1, point2, houses[i]->house_figure[j - 1],
				houses[i]->house_figure[j % house_figure_num], false);
			if (intersect.IsSet())
			{
				return intersect.GetValue();
			}
		}
	}
	return TOptional<FVector>();
}

bool District::create_house(TArray<FVector> given_line, double width,
                            double height, FString obj_type = "House")
{
	if (given_line.Num() < 2)
	{
		return false;
	}
	TArray<FVector> this_figure;
	FVector point1 = AllGeometry::create_segment_at_angle(
		given_line[1], given_line[0], given_line[0], 90, width / 2);
	// this_figure.Add(point0);
	if (!is_point_in_self_figure(point1))
	{
		return false;
	}
	this_figure.Add(point1);
	// House house;
	for (int i = 1; i < given_line.Num(); i++)
	{
		FVector point =
			AllGeometry::create_segment_at_angle(
				given_line[i - 1], given_line[i], given_line[i], -90,
				width / 2);
		if (!is_point_in_self_figure(point))
		{
			return false;
		}
		this_figure.Add(point);
	}
	FVector point2 =
		AllGeometry::create_segment_at_angle(this_figure[given_line.Num() - 1],
		                                     given_line[given_line.Num() - 1],
		                                     given_line[given_line.Num() - 1],
		                                     0, width / 2);
	if (!is_point_in_self_figure(point2))
	{
		return false;
	}
	this_figure.Add(point2);
	for (int i = given_line.Num() - 1; i > 0; i--)
	{
		FVector point = AllGeometry::create_segment_at_angle(given_line[i], given_line[i - 1], given_line[i - 1], -90,
		                                                     width / 2);
		if (!is_point_in_self_figure(point))
		{
			return false;
		}
		this_figure.Add(point);
	}
	int fig_num = this_figure.Num();
	for (int i = 1; i <= fig_num; i++)
	{
		if (is_line_intersect(this_figure[i - 1], this_figure[i % fig_num]).IsSet())
		{
			return false;
		}
	}
	House house(this_figure, height);
	houses.Add(MakeShared<House>(house));
	return true;
}

bool District::create_house(TArray<FVector> given_figure, double height, FString obj_type = "House",
                            bool trustworthy = false)
{
	int fig_num = given_figure.Num();
	if (!trustworthy)
	{
		for (int i = 1; i <= fig_num; i++)
		{
			if (is_line_intersect(given_figure[i - 1], given_figure[i % fig_num]).IsSet() ||
				!is_point_in_self_figure(given_figure[i - 1]))
			{
				return false;
			}
		}
	}
	House house(given_figure, height, obj_type);
	houses.Add(MakeShared<House>(house));
	return true;
}

bool District::attach_district(TSharedPtr<District> other_district,
                               TArray<TSharedPtr<Street>>& streets_to_delete)
{
	TArray<TSharedPtr<Node>> this_figure_new;
	TArray<TSharedPtr<Node>> this_figure;
	TArray<TSharedPtr<Node>> other_figure;
	for (auto p : figure)
	{
		this_figure.AddUnique(p);
	}
	for (auto p : other_district->figure)
	{
		other_figure.AddUnique(p);
	}
	TSharedPtr<Node> my_node = nullptr;
	int current_i = -1;
	int current_j = 0;
	bool is_attachment_found = false;
	bool phase_first_or_second = true;
	bool is_end_found = false;
	int figure_num = this_figure.Num();
	int o_figure_num = other_figure.Num();
	for (int i = 0; i < figure_num; i++)
	{
		for (int j = 0; j < o_figure_num; j++)
		{
			is_attachment_found = false;
			if (this_figure[i] == other_figure[j % o_figure_num])
			{
				is_attachment_found = true;
				current_i = i;
				break;
			}
		}
		if (!is_attachment_found && i != current_i)
		{
			current_i = i;
			break;
		}
	}
	// is_attachment_found = false;
	int max_figure = current_i + figure_num * 2;
	int max_o_figure = current_j + o_figure_num * 2;
	TSharedPtr<Node> first_node = this_figure[current_i];
	this_figure_new.Add(first_node);

	do
	{
		if (phase_first_or_second)
		{
			is_attachment_found = false;
			for (int i = current_i + 1; i < max_figure; i++)
			{
				for (int j = current_j; j < max_o_figure; j++)
				{
					if (this_figure[i % figure_num] == first_node)
					{
						this_figure_new.Add(this_figure[i % figure_num]);
						is_end_found = true;
						break;
					}
					if (this_figure[i % figure_num] == other_figure[j %
						o_figure_num])
					{
						this_figure_new.Add(this_figure[i % figure_num]);
						is_attachment_found = true;
						auto next = this_figure[i % figure_num]->get_next_point(
							this_figure[(i + 1) % figure_num]->get_point());
						if (next.IsSet())
						{
							streets_to_delete.AddUnique(
								next.GetValue()->get_street());
						}
						current_j = j;
						break;
					}
				}
				if (is_attachment_found || is_end_found)
				{
					phase_first_or_second = false;
					break;
				}
				this_figure_new.Add(this_figure[i % figure_num]);
			}
		}
		else
		{
			is_attachment_found = false;
			for (int j = current_j + 1; j < max_o_figure; j++)
			{
				for (int i = current_i; i < max_figure; i++)
				{
					if (other_figure[j % o_figure_num] == first_node)
					{
						this_figure_new.Add(other_figure[i % figure_num]);
						is_end_found = true;
						break;
					}
					if (this_figure[i % figure_num] == other_figure[j %
						o_figure_num])
					{
						this_figure_new.Add(other_figure[j % o_figure_num]);
						is_attachment_found = true;
						auto next = other_figure[j % o_figure_num]->
							get_next_point(
								other_figure[(j + 1) % o_figure_num]->
								get_point());
						if (next.IsSet())
						{
							streets_to_delete.AddUnique(
								next.GetValue()->get_street());
						}
						current_i = i;
						break;
					}
				}
				if (is_attachment_found || is_end_found)
				{
					phase_first_or_second = true;
					break;
				}
				this_figure_new.Add(other_figure[j % o_figure_num]);
			}
		}
	}
	while (!is_end_found);
	if (this_figure_new.Num() == figure_num)
	{
		return false;
	}
	figure = this_figure_new;
	return true;
}

bool District::divide_me(TSharedPtr<District> dist1, TSharedPtr<District> dist2,
                         TSharedPtr<Street> new_seg)
{
	dist1->figure.Empty();
	dist2->figure.Empty();
	// new_seg->street_vertices.Empty();
	int points_num = figure.Num();
	if (points_num <= 3) return false;

	int rand_point = FMath::FRand() * (points_num - 1);
	dist1->figure.Add(figure[rand_point]);
	dist2->figure.Add(figure[rand_point]);
	new_seg->street_vertices.Add(figure[rand_point]);

	for (int i = 1; i < points_num; i++)
	{
		int right = (rand_point - i + points_num) % points_num;
		int left = (rand_point + i + points_num) % points_num;
		if (FMath::Abs(left - right) <= 1)
		{
			if (left == right)
			{
				new_seg->street_vertices.Add(figure[left]);
				dist1->figure.Add(figure[left]);
				dist2->figure.Insert(figure[left], 0);
			}
			else
			{
				dist1->figure.Add(figure[left]);
				dist2->figure.Insert(figure[right], 0);
				bool to_right = FMath::RandBool();
				new_seg->street_vertices.Add(figure[left + to_right]);
				// if (!to_right)
				// {
				// 	dist2->figure.Insert(figure[left],0);
				// }
				// else dist1->figure.Add(figure[right]);
			}
			break;
		}
		TSharedPtr<Node> node(MakeShared<Node>(FVector(
			(figure[left]->get_FVector() + figure[right]->get_FVector() / 2))));
		new_seg->street_vertices.Last()->add_connection(node);
		node->add_connection(new_seg->street_vertices.Last());
		new_seg->street_vertices.Add(node);
		// dist1->figure.Insert(figure[right],0);
		// dist2->figure.Add(figure[left]);
	}
	for (int i = 0; i < new_seg->street_vertices.Num(); i++)
	{
		dist1->figure.Add(
			new_seg->street_vertices[new_seg->street_vertices.Num() - 1 - i]);
		dist1->figure.Add(new_seg->street_vertices[i]);
	}

	// for (auto& node: new_seg->street_vertices)
	// {
	// 	dist1->figure.Insert(node,0);
	// 	dist2->figure.Add(node);
	// }

	return true;
}

bool District::is_adjacent(TSharedPtr<District> other_district)
{
	TArray<TSharedPtr<Node>> this_figure;
	TArray<TSharedPtr<Node>> other_figure;
	for (auto p : figure)
	{
		this_figure.AddUnique(p);
	}
	for (auto p : other_district->figure)
	{
		other_figure.AddUnique(p);
	}
	int figure_num = this_figure.Num();
	int o_figure_num = other_figure.Num();
	int count = 0;
	for (int i = 0; i < figure_num; i++)
	{
		for (int j = 0; j < o_figure_num; j++)
		{
			if (this_figure[i] == other_figure[j % o_figure_num])
			{
				count++;
				if (count == 2)
				{
					return true;
				}
				break;
			}
		}
	}
	return false;
}

Point::~Point()
{
	districts_nearby.Empty();
}

Conn::~Conn()
{
	figure->Empty();
	// street->Empty();
	node.Reset();
}

TOptional<TSharedPtr<Conn>> Node::get_next_point(TSharedPtr<Point> point_)
{
	for (auto c : conn)
	{
		if (c->node->get_FVector() == point_->point)
		{
			return c;
		}
	}
	return TOptional<TSharedPtr<Conn>>();
}

TOptional<TSharedPtr<Conn>> Node::get_prev_point(TSharedPtr<Point> point_)
{
	auto prev_point = get_next_point(point_);
	if (prev_point.IsSet())
	{
		return prev_point.GetValue()->node->get_next_point(point);
	}
	return TOptional<TSharedPtr<Conn>>();
}

void Node::add_connection(const TSharedPtr<Node>& node_)
{
	conn.Add(MakeShared<Conn>(Conn(node_)));
}

void Node::delete_me()
{
	for (auto c : conn)
	{
		for (int i = 0; i < c->node->conn.Num(); i++)
		{
			if (point == c->node->conn[i]->node->get_point())
			{
				c->node->conn.RemoveAt(i);
				break;
			}
		}
	}
}

void Node::print_connections()
{
	UE_LOG(LogTemp, Warning, TEXT("Я: %d"), debug_ind_)
	for (auto& c : conn)
	{
		UE_LOG(LogTemp, Warning, TEXT("---Мое соединение: %d"),
		       c->node->debug_ind_)
	}
}

House::~House()
{
	house_figure.Empty();
}

TArray<TArray<FVector>> House::slice_house(float north, float east, float south, float west)
{
	TArray<TArray<FVector>> house_slices;
	house_slices.SetNum(9);

	if (object_type == "House")
	{
		FVector p1 = house_figure[0];
		FVector p2 = house_figure[1];
		FVector p3 = house_figure[2];
		FVector p4 = house_figure[3];
		FVector w1 = AllGeometry::create_segment_at_angle(p2, p1, p2, 0, west);
		FVector w2 = AllGeometry::create_segment_at_angle(p3, p4, p3, 0, west);
		FVector e1 = AllGeometry::create_segment_at_angle(p1, p2, p1, 0, east);
		FVector e2 = AllGeometry::create_segment_at_angle(p4, p3, p4, 0, east);
		FVector n1 = AllGeometry::create_segment_at_angle(p1, p4, p1, 0, north);
		FVector n2 = AllGeometry::create_segment_at_angle(p2, p3, p2, 0, north);
		FVector s1 = AllGeometry::create_segment_at_angle(p4, p1, p4, 0, south);
		FVector s2 = AllGeometry::create_segment_at_angle(p3, p2, p3, 0, south);

		FVector nw = AllGeometry::create_segment_at_angle(p2, p3, w1, 0, north);
		FVector sw = AllGeometry::create_segment_at_angle(p3, p2, w2, 0, south);
		FVector ne = AllGeometry::create_segment_at_angle(p2, p3, e1, 0, north);
		FVector se = AllGeometry::create_segment_at_angle(p3, p2, e2, 0, south);

		if (e1 != p1 && p1 != n1)
		{
			house_slices[0] = {p1, e1, ne, n1};
		}
		if (e1 != w1 && w1 != ne)
		{
			house_slices[1] = {e1, w1, nw, ne};
		}
		if (p2 != w1 && p2 != n2)
		{
			house_slices[2] = {w1, p2, n2, nw};
		}
		if (n1 != ne && ne != se)
		{
			house_slices[3] = {n1, ne, se, s1};
		}
		house_slices[4] = {ne, nw, sw, se};
		if (nw != n2 && n2 != s2)
		{
			house_slices[5] = {nw, n2, s2, sw};
		}
		if (s1 != se && se != e2)
		{
			house_slices[6] = {s1, se, e2, p4};
		}
		if (se != sw && sw != w2)
		{
			house_slices[7] = {se, sw, w2, e2};
		}
		if (sw != s2 && s2 != p4)
		{
			house_slices[8] = {sw, s2, p4, w2};
		}
	}
	return house_slices;
}

TOptional<FVector> AllGeometry::is_intersect(const FVector& line1_begin,
                                             const FVector& line1_end,
                                             const FVector& line2_begin,
                                             const FVector& line2_end,
                                             bool is_opened)
{
	double dx1 = line1_end.X - line1_begin.X;
	double dy1 = line1_end.Y - line1_begin.Y;
	double dx2 = line2_end.X - line2_begin.X;
	double dy2 = line2_end.Y - line2_begin.Y;

	double det = dx1 * dy2 - dx2 * dy1;

	if (std::abs(det) < 1e-6)
	{
		return TOptional<FVector>();
	}

	double t1 = ((line2_begin.X - line1_begin.X) * dy2 - (line2_begin.Y -
		line1_begin.Y) * dx2) / det;
	double t2 = ((line2_begin.X - line1_begin.X) * dy1 - (line2_begin.Y -
		line1_begin.Y) * dx1) / det;

	if (t1 >= 0.0 && t1 <= 1.0 && t2 >= 0.0 && t2 <= 1.0)
	{
		FVector intersectionPoint(line1_begin.X + t1 * dx1,
		                          line1_begin.Y + t1 * dy1, 0);
		if (is_opened)
		{
			return intersectionPoint;
		}
		if (!is_opened &&
			(FVector::Distance(intersectionPoint, line2_begin) > TNumericLimits<
					double>::Min() &&
				FVector::Distance(intersectionPoint, line2_end) > TNumericLimits
				<double>::Min() &&
				FVector::Distance(intersectionPoint, line1_begin) >
				TNumericLimits<double>::Min() &&
				FVector::Distance(intersectionPoint, line1_end) > TNumericLimits
				<double>::Min()))
		{
			return intersectionPoint;
		}
	}

	return TOptional<FVector>();
}


TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>
AllGeometry::is_intersect_array(
	const TSharedPtr<Node>& line_begin, const TSharedPtr<Node>& line_end,
	const TArray<TSharedPtr<Node>>& lines,
	bool is_opened)
{
	TTuple<TSharedPtr<Node>, TSharedPtr<Node>> point_line;
	double dist = TNumericLimits<double>::Max();
	FVector intersect_point_final(0, 0, 0);
	for (auto& line : lines)
	{
		for (auto& conn : line->conn)
		{
			if (!conn.IsValid()) continue;
			TOptional<FVector> int_point = is_intersect(
				line_begin->get_FVector(), line_end->get_FVector(),
				line->get_FVector(), conn->node->get_FVector(), is_opened);
			if (int_point.IsSet())
			{
				double dist_to_line = FVector::Dist(
					line_begin->get_FVector(), int_point.GetValue());
				if (dist_to_line < dist)
				{
					// point_line = PointLine(line->node, conn->node);
					point_line = TTuple<TSharedPtr<Node>, TSharedPtr<Node>>{
						line, conn->node
					};

					dist = dist_to_line;
					intersect_point_final = int_point.GetValue();
				}
			}
		}
	}
	if (dist == TNumericLimits<double>::Max())
	{
		return TOptional<TTuple<
			FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>();
	}
	TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>> final_tuple{
		intersect_point_final, point_line
	};
	return final_tuple;
}


TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>
AllGeometry::is_intersect_array(
	FVector line_begin, FVector line_end, const TArray<TSharedPtr<Node>>& lines,
	bool is_opened)
{
	TTuple<TSharedPtr<Node>, TSharedPtr<Node>> point_line;
	double dist = TNumericLimits<double>::Max();
	FVector intersect_point_final(0, 0, 0);
	for (auto& line : lines)
	{
		for (auto& conn : line->conn)
		{
			if (!conn.IsValid())continue;
			TOptional<FVector> int_point =
				is_intersect(line_begin, line_end, line->get_FVector(),
				             conn->node->get_FVector(), is_opened);
			if (int_point.IsSet())
			{
				double dist_to_line = FVector::Distance(
					line_begin, int_point.GetValue());
				if (dist_to_line < dist)
				{
					point_line = TTuple<TSharedPtr<Node>, TSharedPtr<Node>>{
						line, conn->node
					};
					dist = dist_to_line;
					intersect_point_final = int_point.GetValue();
				}
			}
		}
	}
	if (dist < TNumericLimits<double>::Max())
	{
		TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>> final_tuple{
			intersect_point_final, point_line
		};
		return final_tuple;
	}
	return TOptional<TTuple<FVector, TTuple<
		                        TSharedPtr<Node>, TSharedPtr<Node>>>>();
}

TOptional<FVector> AllGeometry::is_intersect_array(
	FVector line_begin, FVector line_end,
	const TArray<FVector>& array_point, bool is_opened)
{
	int NumVertices = array_point.Num();
	double dist = TNumericLimits<double>::Max();
	FVector final_point(0, 0, 0);
	for (int i = 1; i <= NumVertices; i++)
	{
		TOptional<FVector> intersect =
			AllGeometry::is_intersect(line_begin, line_end, array_point[i - 1],
			                          array_point[i % NumVertices], is_opened);
		if (intersect.IsSet() && FVector::Distance(
			intersect.GetValue(), line_begin) < dist)
		{
			final_point = intersect.GetValue();
		}
	}
	if (dist < TNumericLimits<double>::Max()) return final_point;
	return TOptional<FVector>();
}

TOptional<TSharedPtr<Node>> AllGeometry::is_intersect_array_clear(
	const TSharedPtr<Node>& line_begin,
	const TSharedPtr<Node>& line_end,
	const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	auto inter_segment = is_intersect_array(line_begin, line_end, lines,
	                                        is_opened);
	if (!inter_segment.IsSet())
	{
		return TOptional<TSharedPtr<Node>>();
	}
	return FVector::Dist(inter_segment->Key,
	                     inter_segment->Value.Key->get_FVector()) <
	       FVector::Dist(inter_segment->Key,
	                     inter_segment->Value.Value->get_FVector())
		       ? inter_segment->Value.Key
		       : inter_segment->Value.Value;
}

int AllGeometry::is_intersect_array_count(const TSharedPtr<Node>& line_begin,
                                          const TSharedPtr<Node>& line_end,
                                          const TArray<TSharedPtr<Node>>& lines,
                                          bool is_opened)
{
	int count = 0;
	for (auto l : lines)
	{
		for (auto& conn : l->conn)
		{
			if (!conn.IsValid())continue;
			if (is_intersect(line_begin->get_FVector(), line_end->get_FVector(),
			                 l->get_FVector(),
			                 conn->node->get_FVector(), is_opened))
			{
				count++;
			}
		}
	}
	return count / 2;
}

TOptional<FVector> AllGeometry::is_intersect_array_clear(
	const FVector& line_begin, const FVector& line_end,
	const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	auto inter_segment = is_intersect_array(line_begin, line_end, lines,
	                                        is_opened);
	if (!inter_segment.IsSet())
	{
		return TOptional<FVector>();
	}
	return FVector::Dist(inter_segment->Key,
	                     inter_segment->Value.Key->get_FVector()) <
	       FVector::Dist(inter_segment->Key,
	                     inter_segment->Value.Value->get_FVector())
		       ? inter_segment->Value.Key->get_FVector()
		       : inter_segment->Value.Value->get_FVector();
}

FVector AllGeometry::create_segment_at_angle(const FVector& line_begin,
                                             const FVector& line_end,
                                             const FVector& line_beginPoint,
                                             double angle_in_degrees,
                                             double length)
{
	FVector line_direction = (line_end - line_begin).GetSafeNormal();
	FVector rotated_direction = line_direction.RotateAngleAxis(
		angle_in_degrees, FVector(0.f, 0.f, 1.f));
	FVector line_endPoint = line_beginPoint + rotated_direction * length;

	return line_endPoint;
}

float AllGeometry::calculate_angle(const FVector& A, const FVector& B,
                                   const FVector& C, bool is_clockwork)
{
	FVector BA = A - B;
	FVector BC = C - B;
	BA.Normalize();
	BC.Normalize();
	double CosTheta = FVector::DotProduct(BA, BC);
	CosTheta = FMath::Clamp(CosTheta, -1.0f, 1.0f);
	double AngleRadians = FMath::Acos(CosTheta);
	if (is_clockwork == true)
	{
		FVector CrossProduct = FVector::CrossProduct(BC, BA);
		if (CrossProduct.Z > 0)
		{
			AngleRadians = 2 * PI - AngleRadians;
		}
	}
	// double

	return FMath::RadiansToDegrees(AngleRadians);
}

float AllGeometry::calculate_angle_clock(const FVector& A, const FVector& B,
                                         const FVector& C, bool is_clockwork)
{
	return calculate_angle(A, B, C, true);
}

float AllGeometry::calculate_angle_counterclock(const FVector& A,
                                                const FVector& B,
                                                const FVector& C,
                                                bool is_clockwork)
{
	return 360 - calculate_angle(A, B, C, true);
}

float AllGeometry::get_poygon_area(const TArray<TSharedPtr<Node>>& Vertices)
{
	int32 NumVertices = Vertices.Num();
	if (NumVertices < 3)
	{
		return 0.0f;
	}

	float Area = 0.0f;

	for (int32 i = 1; i < NumVertices; ++i)
	{
		const FVector& CurrentVertex = Vertices[i - 1]->get_FVector();
		const FVector& NextVertex = Vertices[i]->get_FVector();

		Area += FMath::Abs(
			CurrentVertex.X * NextVertex.Y - CurrentVertex.Y * NextVertex.X);
	}

	Area = Area / 2;

	return Area;
}

float AllGeometry::get_poygon_area(const TArray<TSharedPtr<Point>>& Vertices)
{
	int32 NumVertices = Vertices.Num();
	if (NumVertices < 3)
	{
		return 0.0f;
	}

	float Area = 0.0f;

	for (int32 i = 1; i < NumVertices; ++i)
	{
		const FVector& CurrentVertex = Vertices[i - 1]->point;
		const FVector& NextVertex = Vertices[i]->point;

		Area += FMath::Abs(
			CurrentVertex.X * NextVertex.Y - CurrentVertex.Y * NextVertex.X);
	}

	Area = Area / 2;

	return Area;
}

float AllGeometry::get_poygon_area(const TArray<Point>& Vertices)
{
	auto vertices_new = Vertices;
	if (vertices_new.begin() != vertices_new.end())
	{
		auto new_end = *Vertices.begin();
		vertices_new.Add(new_end);
	}
	int32 NumVertices = vertices_new.Num();
	if (NumVertices < 3)
	{
		return 0.0f;
	}

	float Area = 0.0f;

	for (int32 i = 1; i <= NumVertices; ++i)
	{
		const FVector& CurrentVertex = vertices_new[i - 1].point;
		const FVector& NextVertex = vertices_new[i % NumVertices].point;

		Area += FMath::Abs(
			CurrentVertex.X * NextVertex.Y - CurrentVertex.Y * NextVertex.X);
	}

	Area /= 2;

	return Area;
}

bool AllGeometry::IsConvex(const FVector& Prev, const FVector& Curr,
                           const FVector& Next)
{
	// Проверка на выпуклость вершины
	FVector Edge1 = Curr - Prev;
	FVector Edge2 = Next - Curr;
	return FVector::CrossProduct(Edge1, Edge2).Z <= 0;
}

bool AllGeometry::IsEar(const TArray<FVector> Vertices, int32 PrevIndex,
                        int32 CurrIndex, int32 NextIndex,
                        const TArray<int32> RemainingVertices)
{
	FVector A = Vertices[PrevIndex];
	FVector B = Vertices[CurrIndex];
	FVector C = Vertices[NextIndex];
	TArray<FVector> fig_array{A, B, C};
	for (int Index : RemainingVertices)
	{
		if (Index != PrevIndex && Index != CurrIndex && Index != NextIndex &&
			AllGeometry::IsPointInTriangle(Vertices[Index], A, B, C))
		{
			return false;
		}
	}

	return true;
}

bool AllGeometry::IsPointInTriangle(const FVector& Point, const FVector& A,
                                    const FVector& B, const FVector& C)
{
	auto Sign = [](const FVector& P1, const FVector& P2, const FVector& P3)
	{
		return (P1.X - P3.X) * (P2.Y - P3.Y) - (P2.X - P3.X) * (P1.Y - P3.Y);
	};

	// Вычисляем знаки
	float D1 = Sign(Point, A, B);
	float D2 = Sign(Point, B, C);
	float D3 = Sign(Point, C, A);

	// Проверяем, одинаковы ли знаки (все положительные или все отрицательные)
	bool HasNeg = (D1 < 0) || (D2 < 0) || (D3 < 0);
	bool HasPos = (D1 > 0) || (D2 > 0) || (D3 > 0);

	return !(HasNeg && HasPos);
}

void AllGeometry::TriangulatePolygon(const TArray<FVector>& Vertices,
                                     TArray<int32>& Triangles)
{
	int32 vertice_num = Vertices.Num();
	TArray<int32> RemainingVertices;
	if ((Vertices[0] - Vertices[Vertices.Num() - 1]).Length() < 0.0001)
	{
		vertice_num -= 1;
	}
	if (vertice_num < 3)
	{
		return;
	}
	// Инициализируем все вершины
	for (int32 i = 0; i < vertice_num; i++)
	{
		auto Prev = Vertices[(i + vertice_num - 1) % vertice_num];
		auto Curr = Vertices[i];
		auto Next = Vertices[(i + 1) % vertice_num];
		float Angle = calculate_angle_counterclock(Prev, Curr, Next);
		if (Angle < 179.99 || Angle > 180.01)
		{
			RemainingVertices.Add(i);
		}
	}

	// Основной цикл триангуляции
	while (RemainingVertices.Num() > 2)
	{
		bool EarFound = false;

		for (int32 i = 0; i < RemainingVertices.Num(); i++)
		{
			int32 PrevIndex = RemainingVertices[(i + RemainingVertices.Num() -
				1) % RemainingVertices.Num()];
			int32 CurrIndex = RemainingVertices[i];
			int32 NextIndex = RemainingVertices[(i + 1) % RemainingVertices.
				Num()];

			// Угол проверяем здесь
			float Angle = AllGeometry::calculate_angle_counterclock(
				Vertices[PrevIndex], Vertices[CurrIndex],
				Vertices[NextIndex]);
			if (Angle >= 180.0f)
			{
				continue; // Пропускаем невыпуклые углы
			}

			// Проверка, является ли треугольник ухом
			if (AllGeometry::IsEar(Vertices, PrevIndex, CurrIndex, NextIndex,
			                       RemainingVertices))
			{
				// Добавляем треугольник в результат
				Triangles.Add(PrevIndex);
				Triangles.Add(CurrIndex);
				Triangles.Add(NextIndex);

				// Удаляем текущую вершину из оставшихся
				RemainingVertices.RemoveAt(i);
				EarFound = true;
				break;
			}
		}

		if (!EarFound)
		{
			return;
		}
	}
}

bool AllGeometry::is_point_in_figure(FVector point_, TArray<FVector> figure)
{
	FVector point = point_;
	FVector point2 = point_;
	point2.Y += 5000;
	if (figure.Num() < 3) return false;
	FVector figure0 = figure[0];
	if (figure[0] != figure.Last())
	{
		figure.Add(figure0);
	}
	int fig_num = figure.Num();
	bool intersected = false;
	TOptional<FVector> old_intersec;
	for (int i = 1; i <= fig_num; i++)
	{
		auto intersec = is_intersect(point, point2, figure[i - 1],
		                             figure[i % fig_num], false);
		if (intersec.IsSet())
		{
			intersected = !intersected;
		}
	}
	return intersected;
}

float AllGeometry::point_to_seg_distance(const FVector& SegmentStart,
                                         const FVector& SegmentEnd,
                                         const FVector& Point)
{
	FVector SegmentVector = SegmentEnd - SegmentStart;
	FVector PointVector = Point - SegmentStart;

	float SegmentLengthSquared = SegmentVector.SizeSquared();
	if (SegmentLengthSquared == 0.0f)
	{
		return FVector::Dist(SegmentStart, Point);
	}

	float t = FMath::Clamp(
		FVector::DotProduct(PointVector, SegmentVector) / SegmentLengthSquared,
		0.0f, 1.0f);
	return FVector::Dist(SegmentStart + t * SegmentVector, Point);
}

bool AllGeometry::is_point_near_figure(const TArray<FVector> given_line,
                                       const FVector& Point, double distance)
{
	for (int i = 1; i <= given_line.Num(); i++)
	{
		if (point_to_seg_distance(given_line[i % given_line.Num()],
		                          given_line[i - 1], Point) < distance)
		{
			return true;
		}
	}
	return false;
}

TArray<FVector> AllGeometry::line_to_polygon(const TArray<FVector> given_line,
                                             double width)
{
	if (given_line.Num() < 2)
	{
		return TArray<FVector>();
	}
	TArray<FVector> this_figure;
	FVector point1 = create_segment_at_angle(given_line[1], given_line[0],
	                                         given_line[0], 90, width / 2);
	this_figure.Add(point1);
	// House house;
	for (int i = 1; i < given_line.Num(); i++)
	{
		FVector point =
			create_segment_at_angle(given_line[i - 1], given_line[i],
			                        given_line[i], -90, width / 2);
		this_figure.Add(point);
	}
	FVector point2 =
		create_segment_at_angle(this_figure[given_line.Num() - 2],
		                        given_line[given_line.Num() - 1],
		                        given_line[given_line.Num() - 1], 90,
		                        width / 2);
	this_figure.Add(point2);
	for (int i = given_line.Num() - 1; i > 0; i--)
	{
		FVector point =
			create_segment_at_angle(given_line[i], given_line[i - 1],
			                        given_line[i - 1], -90, width / 2);
		this_figure.Add(point);
	}
	return this_figure;
}

// not sure if it works right
TArray<FVector> AllGeometry::shrink_polygon(const TArray<FVector> cur_polygon, double interval)
{
	// 	shrink_figure_with_roads(
	// 	TArray<TSharedPtr<Node>>& figure_vertices, float road, float main_road)
	// {
	int32 NumVertices = cur_polygon.Num();
	TArray<FVector> new_points;
	auto backup_vertices = cur_polygon;
	if (cur_polygon[0] == cur_polygon[NumVertices - 1])
	{
		NumVertices--;
	}
	if (NumVertices < 3)
	{
		return TArray<FVector>();
	}
	for (int i = 0; i <= NumVertices; ++i)
	{
		auto Prev = (i + NumVertices - 1) % NumVertices;
		auto Curr = i % NumVertices;
		auto Next = (i + 1) % NumVertices;
		float road_height1 = interval;
		float road_height2 = interval;

		FVector parralel1_beg = AllGeometry::create_segment_at_angle(
			cur_polygon[Prev], cur_polygon[Curr], cur_polygon[Prev], 90, road_height1);
		FVector parralel2_beg = AllGeometry::create_segment_at_angle(
			cur_polygon[Curr], cur_polygon[Next], cur_polygon[Next], 90,
			road_height2);
		FVector parralel1_end =
			AllGeometry::create_segment_at_angle(
				cur_polygon[Prev], cur_polygon[Curr], parralel1_beg, 0, 5000);
		FVector parralel2_end =
			AllGeometry::create_segment_at_angle(
				cur_polygon[Next], cur_polygon[Curr], parralel2_beg, 0, 5000);
		parralel1_beg.Z = 0;
		parralel2_beg.Z = 0;
		parralel1_end.Z = 0;
		parralel2_end.Z = 0;
		auto intersection =
			AllGeometry::is_intersect(parralel1_beg, parralel1_end,
			                          parralel2_beg, parralel2_end, false);

		if (intersection.IsSet())
		{
			if (!AllGeometry::is_point_in_figure(intersection.GetValue(), cur_polygon))
			{
				continue;
			}
			auto angle1 =
				AllGeometry::calculate_angle_clock(
					cur_polygon[Prev],
					cur_polygon[Curr],
					cur_polygon[Next]);
			auto angle2 = AllGeometry::calculate_angle_clock(
				parralel1_beg, intersection.GetValue(), parralel2_beg);
			if (angle2 - angle1 < 0.1)
			{
				bool is_valid = true;
				for (int j = 1; j <= cur_polygon.Num(); j++)
				{
					if (AllGeometry::point_to_seg_distance(
						cur_polygon[j - 1],
						cur_polygon[j % cur_polygon.Num()],
						intersection.GetValue()) < interval)
					{
						is_valid = false;
						break;
					}
				}
				if (is_valid == true)
				{
					new_points.Add(cur_polygon[Curr]);
				}
			}
		}
	}

	return new_points;
	// }
}
