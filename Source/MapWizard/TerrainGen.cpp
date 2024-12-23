#include "TerrainGen.h"

void TerrainGen::add_conn(const TSharedPtr<Node>& node1, const TSharedPtr<Node>& node2)
{
	if (node1->get_FVector() == node2->get_FVector())
	{
		return;
	}

	if (!node1->get_next_point(node2->get_point()).IsSet())
	{
		node1->add_connection(node2);
	}
	if (!node2->get_next_point(node1->get_point()).IsSet())
	{
		node2->add_connection(node1);
	}
}
TSharedPtr<Node> TerrainGen::insert_conn(const TSharedPtr<Node>& node1_to_insert,
										 const TSharedPtr<Node>& node2_to_insert, FVector node3_point)
{
	for (int i = 0; i < node1_to_insert->conn.Num(); i++)
	{
		if (node1_to_insert->conn[i]->node == node2_to_insert)
		{
			node1_to_insert->conn.RemoveAt(i);
			break;
		}
	}
	for (int i = 0; i < node2_to_insert->conn.Num(); i++)
	{
		if (node2_to_insert->conn[i]->node == node1_to_insert)
		{
			node2_to_insert->conn.RemoveAt(i);
			break;
		}
	}
	TSharedPtr<Node> new_node(MakeShared<Node>(node3_point));
	add_conn(node1_to_insert, new_node);
	add_conn(node2_to_insert, new_node);
	return new_node;
}

void TerrainGen::move_river(const TSharedPtr<Node>& node1, const TSharedPtr<Node>& node2)
{
	if (node1->is_used() || node2->is_used())
	{
		return;
	}

	FVector point1 = node1->get_FVector();
	FVector backup_point1 = node1->get_FVector();
	point_shift(point1);
	FVector point2 = node2->get_FVector();
	FVector backup_point2 = node2->get_FVector();
	point_shift(point2);

	for (auto conn : node1->conn)
	{
		double dist = FVector::Distance(conn->node->get_FVector(), point1);
		if (dist > max_river_length / 3 * 2 || dist < av_river_length / 3 * 2)
		{
			return;
		}
	}
	for (auto conn : node2->conn)
	{
		double dist = FVector::Distance(conn->node->get_FVector(), point2);
		if (dist > max_river_length || dist < av_river_length / 3 * 2)
		{
			return;
		}
	}
	if (
		// FVector::Distance(point1, point2) > max_river_length / 3 * 2 ||
		FVector::Distance(point1, point2) < av_river_length / 3 * 2)
	{
		return;
	}

	if (!AllGeometry::is_intersect_array(backup_point1, point1, river, false).IsSet() &&
		!AllGeometry::is_intersect_array(backup_point1, point1, map_borders_array, true).IsSet())
	{
		node1->set_FVector(point1);
		// return;
	}
	if (!AllGeometry::is_intersect_array(backup_point2, point2, river, false).IsSet() &&
		!AllGeometry::is_intersect_array(backup_point2, point2, map_borders_array, true).IsSet())
	{
		node2->set_FVector(point2);
		// return;
	}
}

void TerrainGen::move_road(const TSharedPtr<Node>& node)
{
	if (node->is_used())
	{
		return;
	}

	if (node->conn.Num() == 2)
	{
		if (!AllGeometry::is_intersect_array(node->conn[0]->node->get_FVector(), node->conn[1]->node->get_FVector(),
											 river, true)
				 .IsSet() &&
			!AllGeometry::is_intersect_array(node->conn[0]->node->get_FVector(), node->conn[1]->node->get_FVector(),
											 map_borders_array, true)
				 .IsSet() &&
			!AllGeometry::is_intersect_array(node->conn[0]->node->get_FVector(), node->conn[1]->node->get_FVector(),
											 roads, false)
				 .IsSet())
		{
			node->set_FVector((node->conn[0]->node->get_FVector() + node->conn[1]->node->get_FVector()) / 2);
			// return;
		}
	}

	FVector point = node->get_FVector();
	FVector backup_point = node->get_FVector();
	point_shift(point);

	for (auto conn : node->conn)
	{
		if (FVector::Distance(conn->node->get_FVector(), point) > max_road_length)
		{
			return;
		}
	}

	if (!AllGeometry::is_intersect_array(backup_point, point, river, true).IsSet() &&
		!AllGeometry::is_intersect_array(backup_point, point, map_borders_array, true).IsSet() &&
		!AllGeometry::is_intersect_array(backup_point, point, roads, false).IsSet())
	{
		node->set_FVector(point);
		return;
	}
}

void TerrainGen::create_terrain(TArray<TSharedPtr<Node>>& roads_, TArray<District>& figures_array_,
								District& river_figure_, TArray<TSharedPtr<Node>>& map_borders_array_,
								TArray<FVector>& debug_points_array_)
{
	river.Empty();
	roads.Empty();
	road_centers.Empty();
	map_points_array.Empty();
	map_borders_array.Empty();
	guididng_roads_array.Empty();
	weighted_points.Empty();
	figures_array.Empty();

	auto map_node1 = MakeShared<Node>(0, 0, 0);
	auto map_node2 = MakeShared<Node>(0, y_size, 0);
	auto map_node3 = MakeShared<Node>(x_size, y_size, 0);
	auto map_node4 = MakeShared<Node>(x_size, 0, 0);
	add_conn(map_node1, map_node2);
	add_conn(map_node2, map_node3);
	add_conn(map_node3, map_node4);
	add_conn(map_node4, map_node1);
	map_borders_array.Add(map_node4);
	map_borders_array.Add(map_node3);
	map_borders_array.Add(map_node2);
	map_borders_array.Add(map_node1);
	double points_count = 64;
	double av_size = (x_size + y_size) / 2;

	weighted_points.Empty();

	const double point_row_counter = sqrt(points_count);
	for (double x = 1; x < point_row_counter; x++)
	{
		for (int y = 1; y < point_row_counter; y++)
		{
			weighted_points.Add(WeightedPoint{
				FVector(static_cast<int>(av_size / point_row_counter * x +
										 (rand() % (static_cast<int>(av_size / point_row_counter / 2))) -
										 (av_size / point_row_counter / 4)),
						static_cast<int>(av_size / point_row_counter * y) +
							(rand() % (static_cast<int>(av_size / point_row_counter / 2))) -
							(av_size / point_row_counter / 4),
						0),
				(rand() % static_cast<int>(av_size / point_row_counter / 2)) + (av_size / point_row_counter / 3)});
		}
	}

	double StartTime1 = FPlatformTime::Seconds();
	create_guiding_rivers();
	double EndTime1 = FPlatformTime::Seconds();
	double StartTime3 = FPlatformTime::Seconds();
	for (int iter = 0; iter < 100; iter++)
	{
		for (auto& b : bridges)
		{
			move_river(b.Key, b.Value);
		}
	}
	get_river_figure();
	process_bridges();

	double EndTime3 = FPlatformTime::Seconds();
	double StartTime4 = FPlatformTime::Seconds();
	create_guiding_roads();
	double EndTime4 = FPlatformTime::Seconds();
	double StartTime5 = FPlatformTime::Seconds();
	for (int iter = 0; iter < 10; iter++)
	{
		for (auto& r : roads)
		{
			r->set_used(false);
		}

		for (auto& road_center : road_centers)
		{
			road_center->set_used(true);
		}

		for (auto& r : roads)
		{
			move_road(r);
		}
	}
	double EndTime5 = FPlatformTime::Seconds();
	double StartTime6 = FPlatformTime::Seconds();
	int old_nodes = 0;
	while (roads.Num() != old_nodes)
	{
		old_nodes = roads.Num();
		create_usual_roads();
		if (roads.Num() > 2500)
		{
			break;
		}
	}
	double EndTime6 = FPlatformTime::Seconds();
	double StartTime7 = FPlatformTime::Seconds();
	for (int i = 0; i < 2; i++)
	{
		for (auto& r : roads)
		{
			r->set_used(false);
		}

		for (auto& road_center : road_centers)
		{
			road_center->set_used(true);
		}
		for (auto& r : roads)
		{
			move_road(r);
		}
	}

	double EndTime7 = FPlatformTime::Seconds();

	double StartTime8 = FPlatformTime::Seconds();
	shrink_roads();
	double EndTime8 = FPlatformTime::Seconds();
	double StartTime9 = FPlatformTime::Seconds();
	get_closed_figures(roads, figures_array, 200);
	double EndTime9 = FPlatformTime::Seconds();
	double StartTime10 = FPlatformTime::Seconds();
	process_blocks(figures_array);
	double EndTime10 = FPlatformTime::Seconds();

	for (int i = 0; i < roads.Num(); i++)
	{
		if (roads[i]->conn.Num() != roads[i]->get_point()->blocks_nearby.Num())
		{
			FVector def_point = roads[i]->get_FVector();
			debug_points_array_.Add(def_point);
		}
	}

	double StartTime11 = FPlatformTime::Seconds();
	for (auto& b : figures_array)
	{
		process_houses(b);
	}
	double EndTime11 = FPlatformTime::Seconds();
	[[maybe_unused]] double time1 = EndTime1 - StartTime1;
	[[maybe_unused]] double time3 = EndTime3 - StartTime3;
	[[maybe_unused]] double time4 = EndTime4 - StartTime4;
	[[maybe_unused]] double time5 = EndTime5 - StartTime5;
	[[maybe_unused]] double time6 = EndTime6 - StartTime6;
	[[maybe_unused]] double time7 = EndTime7 - StartTime7;
	[[maybe_unused]] double time8 = EndTime8 - StartTime8;
	[[maybe_unused]] double time9 = EndTime9 - StartTime9;
	[[maybe_unused]] double time10 = EndTime10 - StartTime10;
	[[maybe_unused]] double time11 = EndTime11 - StartTime11;
	figures_array_ = figures_array;
	river_figure_ = river_figure;
	map_borders_array_ = map_borders_array;
	roads_ = roads;
}

void TerrainGen::create_guiding_rivers()
{
	FVector start_river_point(0, 0, 0);
	FVector end_river_point(0, y_size, 0);
	auto start_point = MakeShared<Node>((start_river_point + end_river_point) / 2);

	FVector end_river =
		AllGeometry::create_segment_at_angle(FVector(0, 0, 0), FVector{0, y_size, 0}, start_point->get_FVector(),
											 -90 + (rand() % 20), (rand() % 20 + 10) * (av_river_length));

	auto end_point = MakeShared<Node>(end_river);
	auto start_point_left = MakeShared<Node>(FVector(0, y_size / 2 - av_river_length / 2, 0));
	auto start_point_right = MakeShared<Node>(FVector(0, y_size / 2 + av_river_length / 2, 0));
	river.Add(start_point_left);
	river.Add(start_point_right);
	// guiding_river.Add(start_point);
	add_conn(start_point_left, start_point_right);
	create_guiding_river_segment(start_point, end_point, start_point_left, start_point_right);
}

void TerrainGen::create_guiding_river_segment(const TSharedPtr<Node>& start_point, const TSharedPtr<Node>& end_point,
											  const TSharedPtr<Node>& start_point_left,
											  const TSharedPtr<Node>& start_point_right)
{
	guiding_river.AddUnique(start_point);
	bool is_ending = false;

	auto intersect_river = AllGeometry::is_intersect_array(start_point, end_point, guiding_river, false);
	auto intersect_border = AllGeometry::is_intersect_array(start_point, end_point, map_borders_array, false);
	if (intersect_border.IsSet())
	{
		is_ending = true;
		end_point->set_FVector(intersect_border->Key);
	}
	else if (intersect_river.IsSet())
	{
		is_ending = true;
		end_point->set_FVector(intersect_river->Key);
	}
	TSharedPtr<Node> end_point_left = MakeShared<Node>(AllGeometry::create_segment_at_angle(
		start_point->get_FVector(), end_point->get_FVector(), end_point->get_FVector(), -90, av_river_length / 2));
	TSharedPtr<Node> end_point_right = MakeShared<Node>(AllGeometry::create_segment_at_angle(
		start_point->get_FVector(), end_point->get_FVector(), end_point->get_FVector(), 90, av_river_length / 2));
	river.AddUnique(end_point_left);
	river.AddUnique(end_point_right);

	if (intersect_border.IsSet())
	{
		end_point_left->set_used();
		end_point_right->set_used();
	}

	TSharedPtr<Node> old_node = start_point;
	TSharedPtr<Node> old_node_left = start_point_left;
	TSharedPtr<Node> old_node_right = start_point_right;
	add_conn(start_point, old_node);
	int dist_times = FVector::Distance(start_point->get_FVector(), end_point->get_FVector()) / (av_river_length);
	for (int i = 1; i <= dist_times; i++)
	{
		bridges.Add(MakeTuple(old_node_left, old_node_right));
		auto node_ptr = MakeShared<Node>();
		TSharedPtr<Node> node_ptr_left =
			MakeShared<Node>(start_point_left->get_FVector() +
							 ((end_point_left->get_FVector() - start_point_left->get_FVector()) / dist_times * i));
		TSharedPtr<Node> node_ptr_right =
			MakeShared<Node>(start_point_right->get_FVector() +
							 ((end_point_right->get_FVector() - start_point_right->get_FVector()) / dist_times * i));
		node_ptr_left->set_type(point_type::river);
		node_ptr_right->set_type(point_type::river);
		if (i != dist_times)
		{
			create_segment(river, node_ptr_left, old_node_left, true, point_type::river, max_river_length);
			create_segment(river, node_ptr_right, old_node_right, true, point_type::river, max_river_length);
		}
		else
		{
			node_ptr_left = end_point_left;
			node_ptr_right = end_point_right;
			create_segment(river, end_point_left, old_node_left, true, point_type::river, max_river_length);
			create_segment(river, end_point_right, old_node_right, true, point_type::river, max_river_length);
			// end_point->add_connection(old_node, false);
			// old_node->add_connection(end_point, false);
		}
		river.AddUnique(node_ptr_left);
		river.AddUnique(node_ptr_right);
		// create_segment(river, node_ptr_left, node_ptr_right, true, point_type::river, max_river_length);
		old_node_left = node_ptr_left;
		old_node_right = node_ptr_right;
	}

	if (!is_ending)
	{
		Node next_segment = Node(AllGeometry::create_segment_at_angle(
			start_point->get_FVector(), end_point->get_FVector(), end_point->get_FVector(), -60 + (rand() % 120),
			(rand() % 20 + 10) * (av_river_length)));
		create_guiding_river_segment(end_point, MakeShared<Node>(next_segment), old_node_left, old_node_right);
		if (rand() % 4 >= 3)
		{
			did_river_multiplied = true;
			auto next_segment1 = MakeShared<Node>(Node(AllGeometry::create_segment_at_angle(
				start_point->get_FVector(), end_point->get_FVector(), end_point->get_FVector(), -60 + (rand() % 120),
				(rand() % 20 + 10) * (av_river_length))));
			create_guiding_river_segment(end_point, next_segment1, old_node_left, old_node_right);
		}
		if (rand() % 8 >= 7)
		{
			did_river_multiplied = true;
			auto next_segment2 = MakeShared<Node>(Node(AllGeometry::create_segment_at_angle(
				start_point->get_FVector(), end_point->get_FVector(), end_point->get_FVector(), -60 + (rand() % 120),
				(rand() % 20 + 10) * (av_river_length))));
			create_guiding_river_segment(end_point, next_segment2, old_node_left, old_node_right);
		}
	}
	else
	{
		add_conn(old_node_left, old_node_right);
	}
}
void TerrainGen::process_bridges()
{
	bridges.RemoveAll(
		[&](const TTuple<TSharedPtr<Node>, TSharedPtr<Node>>& A)
		{
			return rand() % 5 >= 4 ||
				FVector::Distance(A.Key->get_FVector(), A.Value->get_FVector()) > (max_river_length * 3 / 2) ||
				AllGeometry::is_intersect_array_count(A.Key, A.Value, river, true) % 2 != 0;
		});

	Algo::Sort(
		bridges,
		[](const TTuple<TSharedPtr<Node>, TSharedPtr<Node>>& A, const TTuple<TSharedPtr<Node>, TSharedPtr<Node>>& B)
		{
			return FVector::Distance(A.Key->get_FVector(), A.Value->get_FVector()) <
				FVector::Distance(B.Key->get_FVector(), B.Value->get_FVector());
		});
}


void TerrainGen::create_guiding_roads()
{
	// double av_size = (x_size+y_size)/2;
	constexpr double point_row_counter = 9;
	for (double x = 1; x < point_row_counter; x++)
	{
		for (double y = 1; y < point_row_counter; y++)
		{
			auto x_val = x_size / point_row_counter * x +
				(rand() % (static_cast<int>(x_size / point_row_counter / 2))) - (x_size / point_row_counter / 4);
			auto y_val = y_size / point_row_counter * y +
				(rand() % (static_cast<int>(x_size / point_row_counter / 2))) - (x_size / point_row_counter / 4);
			roads.Add(MakeShared<Node>(Node{FVector(x_val, y_val, 0)}));
		}
	}
	for (auto& r : roads)
	{
		for (int iter = 0; iter < 10; iter++)
		{
			point_shift(r->get_point()->point);
		}
	}
	TArray<TTuple<double, TSharedPtr<Node>>> weighted_road_centers;
	for (auto& r : roads)
	{
		if (FVector::Distance(r->get_FVector(), FVector{x_size / 2, y_size / 2, 0}) < (y_size / 3))
		{
			bool is_break = false;
			for (auto riv : river)
			{
				if (FVector::Distance(r->get_FVector(), riv->get_FVector()) < river_road_distance)
				{
					is_break = true;
					break;
				}
			}
			if (is_break)
			{
				continue;
			}
			double weight = 0;
			for (auto& w : weighted_points)
			{
				if (FVector::Distance(r->get_FVector(), w.point) < w.weight)
				{
					weight += (w.weight - FVector::Distance(r->get_FVector(), w.point));
				}
			}
			weighted_road_centers.Add(TTuple<double, TSharedPtr<Node>>{weight, r});
		}
	}

	Algo::Sort(weighted_road_centers,
			   [](const TTuple<double, TSharedPtr<Node>>& A, const TTuple<double, TSharedPtr<Node>>& B)
			   { return A.Get<0>() < B.Get<0>(); });
	roads.Reset();
	// road.Add(weighted_road_centers[0].Value);
	for (int i = 1; i < weighted_road_centers.Num(); i++)
	{
		bool is_break = false;
		for (auto r : roads)
		{
			if (FVector::Distance(weighted_road_centers[i].Value->get_FVector(), r->get_FVector()) < (y_size / 7))
			{
				is_break = true;
				break;
			}
		}
		if (!is_break)
		{
			roads.Add(weighted_road_centers[i].Value);
		}
	}
	// placing bridges
	int bridges_num = 15;
	if (bridges_num > bridges.Num())
	{
		bridges_num = bridges.Num();
	}
	for (int i = 0; i < bridges_num; i++)
	{
		auto bridge1 = MakeShared<Node>(AllGeometry::create_segment_at_angle(
			bridges[i].Key->get_FVector(), bridges[i].Value->get_FVector(), bridges[i].Value->get_FVector(), 0, 20));
		auto bridge2 = MakeShared<Node>(AllGeometry::create_segment_at_angle(
			bridges[i].Value->get_FVector(), bridges[i].Key->get_FVector(), bridges[i].Key->get_FVector(), 0, 20));
		add_conn(bridge1, bridge2);
		roads.Add(bridge1);
		roads.Add(bridge2);
	}

	for (auto r : roads)
	{
		weighted_points.Add(WeightedPoint{r->get_FVector(), -y_size / 7});
	}

	for (auto& r : roads)
	{
		road_centers.Add(r);
	}

	for (auto& r : road_centers)
	{
		r->set_type(point_type::main_road);
	}
	if (city_plan == ECityPlan::combined)
	{
		for (int i = 0; i < road_centers.Num() - 1; i++)
		{
			auto local_road = road_centers;

			local_road.Sort(
				[i, this](const TSharedPtr<Node>& Item1, const TSharedPtr<Node>& Item2)
				{
					return FVector::Distance(road_centers[i]->get_FVector(), Item1->get_FVector()) <
						FVector::Distance(road_centers[i]->get_FVector(), Item2->get_FVector());
				});
			int success_roads = 0;
			for (int j = 0; j < local_road.Num() - 1 && success_roads < 4; j++)
			{
				success_roads += create_guiding_road_segment(road_centers[i], local_road[j]);
			}
		}
	}
	else if (city_plan == ECityPlan::radial)
	{
		TSharedPtr<Node> closest_center;
		float closest_dist = TNumericLimits<float>::Max();
		for (auto r : road_centers)
		{
			if (FVector::Distance(r->get_FVector(), center) < closest_dist &&
				!river_figure.is_point_in_figure(r->get_FVector()))
			{
				closest_dist = FVector::Distance(r->get_FVector(), center);
				closest_center = r;
			}
		}
		road_centers.Empty();
		road_centers.Add(closest_center);
		int beams_num = (rand() % 4) + 3;
		auto left_node = MakeShared<Node>(closest_center->get_FVector());
		left_node->set_FVector(FVector(closest_center->get_FVector().X, 0, 0));

		for (int i = 1; i <= beams_num; i++)
		{
			auto new_node_fvec = AllGeometry::create_segment_at_angle(
				left_node->get_FVector(), closest_center->get_FVector(), closest_center->get_FVector(),
				360 / i * beams_num, av_distance / 1.5);
			TSharedPtr<Node> new_node(MakeShared<Node>(new_node_fvec));
			new_node->set_type(point_type::main_road);
			roads.Add(new_node);
			if (create_guiding_road_segment(closest_center, new_node))
			{
				road_centers.Add(new_node);
			}
		}
	}
	else if (city_plan == ECityPlan::radial_circle)
	{
		TSharedPtr<Node> closest_center;
		float closest_dist = TNumericLimits<float>::Max();
		for (auto r : road_centers)
		{
			if (FVector::Distance(r->get_FVector(), center) < closest_dist &&
				!river_figure.is_point_in_figure(r->get_FVector()))
			{
				closest_dist = FVector::Distance(r->get_FVector(), center);
				closest_center = r;
			}
		}
		road_centers.Empty();
		road_centers.Add(closest_center);
		auto left_node = MakeShared<Node>(closest_center->get_FVector());
		left_node->set_FVector(FVector(closest_center->get_FVector().X, 0, 0));

		TArray<TSharedPtr<Node>> circle_nodes;
		for (int general_angle = 0, i = 0; general_angle < 360; general_angle += (rand() % 20 + 10), i++)
		{
			auto new_node_fvec = AllGeometry::create_segment_at_angle(
				left_node->get_FVector(), closest_center->get_FVector(), closest_center->get_FVector(), general_angle,
				static_cast<double>(av_distance / 2));
			borders_array.Add(new_node_fvec);
			auto new_node = MakeShared<Node>(new_node_fvec);
			new_node->set_type(point_type::wall);
			road_centers.Add(new_node);
			if (i != 0)
			{
				create_guiding_road_segment(new_node, circle_nodes[i - 1]);
			}
			if (rand() % 360 < 45 || i == 0)
			{
				new_node->set_type(point_type::main_road);
				create_guiding_road_segment(closest_center, new_node);
			}
			circle_nodes.Add(new_node);
		}
		create_guiding_road_segment(circle_nodes[0], circle_nodes[circle_nodes.Num() - 1]);

		borders_array.Add(circle_nodes[0]->get_FVector());
	}
}

bool TerrainGen::create_guiding_road_segment(const TSharedPtr<Node>& start_point, const TSharedPtr<Node>& end_point)
{
	if (AllGeometry::is_intersect_array(start_point, end_point, river, true).IsSet())
	{
		return false;
	}

	TSharedPtr<Node> old_node = start_point;
	// river.Add(old_node);
	int dist_times = FVector::Distance(start_point->get_FVector(), end_point->get_FVector()) / (av_road_length);
	for (int i = 1; i <= dist_times; i++)
	{
		TSharedPtr<Node> node = MakeShared<Node>(0, 0, 0);
		node->set_type(point_type::main_road);
		node->set_FVector(start_point->get_FVector() +
						  ((end_point->get_FVector() - start_point->get_FVector()) / dist_times * i));
		if (i == dist_times)
		{
			node = end_point;
		}
		create_segment(roads, old_node, node, true, start_point->get_type(), max_road_length);
		// add_conn(node, old_node);
		roads.AddUnique(node);
		old_node = node;
	}
	return true;
}

void TerrainGen::shrink_roads()
{
	int road_points = roads.Num();
	int old_road_points = TNumericLimits<int>::Max();
	while (road_points != old_road_points)
	{
		old_road_points = roads.Num();
		auto array = roads;
		roads.RemoveAll(
			[&](const TSharedPtr<Node>& node)
			{
				TSharedPtr<Node> target_node = nullptr;
				for (auto conn : node->conn)
				{
					if (FVector::Dist(node->get_FVector(), conn->node->get_FVector()) < min_road_length)
					{
						target_node = conn->node;
						break;
					}
				}
				if (target_node.IsValid())
				{
					for (auto del_node : node->conn)
					{
						add_conn(del_node->node, target_node);
					}
					node->delete_me();
					return true;
				}
				return false;
			});
		road_points = roads.Num();
	}
	road_points = roads.Num();
	old_road_points = TNumericLimits<int>::Max();
	while (road_points != old_road_points)
	{
		old_road_points = roads.Num();
		roads.RemoveAll(
			[&](const TSharedPtr<Node>& node)
			{
				if (node->conn.Num() < 2)
				{
					node->delete_me();
					return true;
				}
				return false;
			});
		road_points = roads.Num();
	}
}

void TerrainGen::create_usual_roads()
{
	roads.Sort([this](auto Item1, auto Item2) { return FMath::FRand() < 0.5f; });
	TArray<TSharedPtr<Node>> add_road = roads;


	for (auto road_node : roads)
	{
		if (!road_node->is_used())
		{
			if (road_node->conn.Num() == 1)
			{
				if (rand() % 100 <= road_forward_chance)
				{
					auto length = FVector::Distance(road_node->get_FVector(), road_node->conn[0]->node->get_FVector()) +
						(rand() % 40) - 20;
					if (length < min_new_road_length)
					{
						length = min_new_road_length;
					}
					if (length > max_road_length)
					{
						length = max_road_length;
					}
					double angle_in_degrees = (rand() % 10) - 5;
					auto line1 = AllGeometry::create_segment_at_angle(
						road_node->conn[0]->node->get_FVector(), road_node->get_FVector(), road_node->get_FVector(),
						angle_in_degrees, length);

					TSharedPtr<Node> new_node = MakeShared<Node>(line1);
					new_node->set_type(point_type::road);
					bool is_possible = false;

					if (!borders_array.IsEmpty())
					{
						if (AllGeometry::is_point_in_figure(line1, borders_array))
						{
							is_possible = true;
						}
					}
					else
					{
						for (auto rc : road_centers)
						{
							if (FVector::Distance(rc->get_FVector(), line1) < rc->conn.Num() * (y_size / 28))
							{
								is_possible = true;
								break;
							}
						}
					}
					if (is_possible)
					{
						// create_usual_road_segment(add_road, road_node, new_node);
						create_segment(add_road, road_node, new_node, false, point_type::road, max_road_length);
					}
				}
			}
			if (road_node->conn.Num() == 2 || road_node->conn.Num() == 1)
			{
				if (rand() % 100 <= road_left_chance)
				{
					auto length = FVector::Distance(road_node->get_FVector(), road_node->conn[0]->node->get_FVector()) +
						(rand() % 40) - 20;
					if (length < min_new_road_length)
					{
						length = min_new_road_length;
					}
					if (length > max_road_length)
					{
						length = max_road_length;
					}
					double angle_in_degrees = 90 + (rand() % 10) - 5;
					auto line2 = AllGeometry::create_segment_at_angle(
						road_node->conn[0]->node->get_FVector(), road_node->get_FVector(), road_node->get_FVector(),
						angle_in_degrees, length);
					TSharedPtr<Node> new_node2 = MakeShared<Node>(line2);
					new_node2->set_type(point_type::road);
					bool is_possible = false;
					for (auto rc : road_centers)
					{
						if (FVector::Distance(rc->get_FVector(), line2) < rc->conn.Num() * (y_size / 28))
						{
							is_possible = true;
							break;
						}
					}
					if (is_possible)
					{
						// create_usual_road_segment(add_road, road_node, new_node2);
						create_segment(add_road, road_node, new_node2, false, point_type::road, max_road_length);
					}
				}
				if (rand() % 100 <= road_right_chance)
				{
					auto length = FVector::Distance(road_node->get_FVector(), road_node->conn[0]->node->get_FVector()) +
						(rand() % 40) - 20;
					if (length < min_new_road_length)
					{
						length = min_new_road_length;
					}
					if (length > max_road_length)
					{
						length = max_road_length;
					}
					double angle_in_degrees = -90 + (rand() % 10) - 5;
					auto line3 = AllGeometry::create_segment_at_angle(
						road_node->conn[0]->node->get_FVector(), road_node->get_FVector(), road_node->get_FVector(),
						angle_in_degrees, length);
					TSharedPtr<Node> new_node3 = MakeShared<Node>(line3);
					new_node3->set_type(point_type::road);
					bool is_possible = false;
					for (auto rc : road_centers)
					{
						if (FVector::Distance(rc->get_FVector(), line3) < rc->conn.Num() * (y_size / 28))
						{
							is_possible = true;
							break;
						}
					}
					if (is_possible)
					{
						// create_usual_road_segment(add_road, road_node, new_node3);
						create_segment(add_road, road_node, new_node3, false, point_type::road, max_road_length);
					}
				}
			}
		}

		road_node->set_used();
	}
	for (auto a : add_road)
	{
		bool is_in_road = false;
		for (auto r : roads)
		{
			if (a->get_FVector() == r->get_FVector())
			{
				is_in_road = true;
				// a = r;
				break;
			}
		}
		if (!is_in_road)
		{
			roads.AddUnique(a);
		}
	}
	// roads += add_road;

	for (int i = 0; i < roads.Num(); i++)
	{
		if (!roads[i]->is_used())
		{
			for (int count = 0; count < 3; count++)
			{
				move_road(roads[i]);
			};
		}
	}
}

TOptional<TSharedPtr<Node>> TerrainGen::create_segment(TArray<TSharedPtr<Node>>& array, TSharedPtr<Node> start_point,
													   TSharedPtr<Node> end_point, bool to_exect_point, point_type type,
													   double max_length) const
{
	TSharedPtr<Node> backup_endpoint = end_point;
	do
	{
		end_point = backup_endpoint;
		bool force_end = false;
		if ((end_point->get_FVector().X) < 0)
		{
			end_point->set_FVector_X(0);
			force_end = true;
		}
		if ((end_point->get_FVector().Y) < 0)
		{
			end_point->set_FVector_Y(0);
			force_end = true;
		}
		if ((end_point->get_FVector().X) > x_size)
		{
			end_point->set_FVector_X(x_size);
			force_end = true;
		}
		if ((end_point->get_FVector().Y) > y_size)
		{
			end_point->set_FVector_Y(y_size);
			force_end = true;
		}
		end_point->set_type(type);
		if (force_end)
		{
			return end_point;
		}
		int tries = 0;
		auto intersection = AllGeometry::is_intersect_array_clear(start_point, end_point, array, false);

		while (intersection.IsSet())
		{
			if (intersection.GetValue()->get_type() == point_type::wall)
			{
				return TSharedPtr<Node>();
			}
			end_point = intersection.GetValue();
			tries++;
			if (tries > 10 || FVector::Distance(start_point->get_FVector(), end_point->get_FVector()) > max_length
				// ||				to_exect_point
			)
			{
				// auto presise_intersect = AllGeometry::is_intersect_array(start_point->get_point(),
				// backup_endpoint->get_point(), array, false);
				// if (presise_intersect.IsSet())
				// {
				// 	end_point =
				// insert_conn(presise_intersect->Value.Key,
				// presise_intersect->Value.Value,
				// 							presise_intersect->Key);
				// }
				break;
			}
			intersection = AllGeometry::is_intersect_array_clear(start_point, end_point, array, false);
		}
		if (tries > 10 || FVector::Distance(start_point->get_FVector(), end_point->get_FVector()) > max_length ||
			to_exect_point)
		{
			auto presise_intersect = AllGeometry::is_intersect_array(start_point->get_FVector(),
																	 backup_endpoint->get_FVector(), array, false);
			if (presise_intersect.IsSet())
			{
				end_point =
					insert_conn(presise_intersect->Value.Key, presise_intersect->Value.Value, presise_intersect->Key);
			}
		}
		TOptional<FVector> river_intersection;
		if (type != point_type::river)
		{
			river_intersection = AllGeometry::is_intersect_array_clear(start_point->get_FVector(),
																	   end_point->get_FVector(), river, true);
		}
		if (!river_intersection.IsSet())
		{
			add_conn(start_point, end_point);
			array.AddUnique(end_point);
		}
		else
		{
			return TOptional<TSharedPtr<Node>>();
		}
		if (!to_exect_point)
		{
			return end_point;
		}
		start_point = end_point;
	}
	while (end_point->get_FVector() != backup_endpoint->get_FVector());

	if (end_point->get_FVector() == backup_endpoint->get_FVector())
	{
		end_point = backup_endpoint;
	}
	return end_point;
}

void TerrainGen::point_shift(FVector& point)
{
	FVector bias(0, 0, 0);
	double biggest_weight = 0;
	for (int j = 0; j < weighted_points.Num(); j++)
	{
		double length = FVector::Distance(point, weighted_points[j].point);
		if (length < abs(weighted_points[j].weight) && biggest_weight < weighted_points[j].weight - length)
		{
			biggest_weight = weighted_points[j].weight - length;
			bias = ((point - weighted_points[j].point) / length) * (weighted_points[j].weight - length) / 50;
		}
	}
	point += bias;
}

void TerrainGen::get_closed_figures(TArray<TSharedPtr<Node>> lines, TArray<District>& fig_array, int figure_threshold)
{
	bool is_river = false;
	if (lines.Num() > 0 && lines[0]->get_type() == point_type::river)
	{
		is_river = true;
	}
	for (auto l : lines)
	{
		for (auto lconn : l->conn)
		{
			if (!lconn->figure->IsEmpty() || lconn->not_in_figure)
			{
				// UE_LOG(LogTemp, Warning, TEXT("Продолжили!"));
				continue;
			}
			// UE_LOG(LogTemp, Warning, TEXT("----------Начал вывод фигуры, размер фигуры %d"), lines.Num());
			TSharedPtr<TArray<TSharedPtr<Point>>> figure_array = MakeShared<TArray<TSharedPtr<Point>>>();
			TArray<TSharedPtr<Conn>> conn_array;
			conn_array.Add(lconn);
			auto first_node = l;

			// first_node->print_connections();
			auto second_node = lconn->node;
			// second_node->print_connections();
			figure_array->Add(l->get_point());
			figure_array->Add(lconn->node->get_point());
			TSharedPtr<Node> rightest_node;
			TSharedPtr<Conn> this_conn;
			bool some_error = false;
			bool not_in_figure = false;
			double general_angle = 0;
			// while (!figure_array->Contains(second_node))
			while (second_node->get_FVector() != l->get_point()->point)
			{
				double smallest_angle = 360;
				for (int i = 0; i < second_node->conn.Num(); i++)
				{
					double angle =
						AllGeometry::calculate_angle(second_node->conn[i]->node->get_FVector(),
													 second_node->get_FVector(), first_node->get_FVector(), true);

					if (angle < smallest_angle && angle > 1)
					{
						smallest_angle = angle;
						rightest_node = second_node->conn[i]->node;
						this_conn = second_node->conn[i];
					}
				}
				general_angle += smallest_angle;
				if (smallest_angle == 360)
				{
					not_in_figure = true;
					break;
				}

				// UE_LOG(LogTemp, Warning, TEXT("Добавили точку %f, %f, index %d"), rightest_node->get_node()->point.X,
				// 	   rightest_node->get_node()->point.Y, rightest_node->debug_ind_);
				// rightest_node->print_connections();
				if (!this_conn->figure->IsEmpty())
				{
					some_error = true;
					break;
				}
				figure_array->Add(rightest_node->get_point());
				conn_array.Add(this_conn);

				if (figure_array->Num() > figure_threshold)
				{
					some_error = true;
					break;
				}
				first_node = second_node;
				second_node = rightest_node;
			}
			if (some_error)
			{
				continue;
			}
			else if (not_in_figure)
			{
				for (auto conn : conn_array)
				{
					conn->not_in_figure = true;
				}
			}
			else
			{
				for (auto conn : conn_array)
				{
					conn->figure = figure_array;
				}
				if ((general_angle / (figure_array->Num() - 1) < 180 && !is_river) ||
					(is_river && (general_angle / (figure_array->Num() - 1) > 180)))
				{
					// UE_LOG(LogTemp, Warning, TEXT("Фигура добавлена, размер %d"), figure_array->Num());
					fig_array.Add(District(*figure_array));
				}
			}
		}
	}
}
void TerrainGen::get_river_figure()
{
	TArray<District> river_fig_array;
	get_closed_figures(river, river_fig_array, 1000);
	if (river_fig_array.IsEmpty())
	{
		return;
	}
	river_fig_array.Sort([this](const District& Item1, const District& Item2)
						 { return Item1.figure.Num() > Item2.figure.Num(); });

	river_figure = river_fig_array[0];
	river.RemoveAll(
		[&](const TSharedPtr<Node>& A)
		{
			for (auto exist_f : river_figure.figure)
			{
				if (A->get_FVector() == exist_f->point)
				{
					return false;
				}
			}
			A->delete_me();
			return true;
		});

	river.RemoveAll(
		[&](const TSharedPtr<Node>& r)
		{
			for (int i = 1; i < river_figure.figure.Num() - 1; i++)
			{
				if (r->conn.Num() > 2)
				{
					TSharedPtr<Node> prev;
					TSharedPtr<Node> next;
					for (auto c : r->conn)
					{
						if (c->node->get_FVector() == river_figure.figure[i - 1]->point)
						{
							prev = c->node;
						}
						if (c->node->get_FVector() == river_figure.figure[i + 1]->point)
						{
							next = c->node;
						}
					}
					if (prev && next)
					{
						add_conn(prev, next);
						r->delete_me();
						return true;
					}
				}
			}
			return false;
		});

	bridges.RemoveAll(
		[&](const TTuple<TSharedPtr<Node>, TSharedPtr<Node>>& A)
		{
			bool is_key_in = false;
			bool is_value_in = false;
			for (auto river_point : river)
			{
				if (A.Key->get_FVector() == river_point->get_FVector())
				{
					is_key_in = true;
					continue;
				}
				if (A.Value->get_FVector() == river_point->get_FVector())
				{
					is_value_in = true;
					continue;
				}
			}
			return !is_key_in || !is_value_in;
		});
}
// void TerrainGen::smooth_blocks(TArray<Block>& blocks)
// {
// 	for (auto& b : blocks)
// 	{
// 		int fig_num = b.figure.Num();
// 		for (int i = 1; i <= fig_num; i++)
// 		{
// 			if (AllGeometry::calculate_angle(b.figure[i - 1]->point, b.figure[i % fig_num]->point,
// 											 b.figure[(i + 1) % fig_num]->point, true) > 180)
// 			{
// 				b.figure[i % fig_num]->point = (b.figure[i - 1]->point + b.figure[(i + 1) % fig_num]->point) / 2;
// 			}
// 		}
// 		b.area = AllGeometry::get_poygon_area(b.figure);
// 	}
// }
void TerrainGen::process_blocks(TArray<District>& blocks)
{
	blocks.RemoveAll([this](District& Item1) { return !(Item1.shrink_size(Item1.self_figure, 3.0f, 6.0)); });

	blocks.Sort([this](const District& Item1, const District& Item2) { return Item1.area > Item2.area; });
	double royal_area = 0;
	bool royal_found = false;
	// bool dock_found = false;
	for (auto& b : blocks)
	{
		bool is_river = false;
		for (auto r : river)
		{
			if (b.is_point_in_self_figure(r->get_FVector()))
			{
				is_river = true;
				b.set_type(block_type::dock);
				break;
			}
		}
		if (b.get_type() == block_type::unknown && !royal_found && royal_area == 0)
		{
			bool point1 = false;
			int is_in_main = 0;
			for (auto& p : b.figure)
			{
				if (is_river)
				{
					break;
				}
				if (FVector::Distance(p->point, center) < (x_size + y_size) / 10)
				{
					point1 = true;
				}
				if (p->type == point_type::main_road)
				{
					is_in_main++;
				}
				if (point1 && is_in_main >= 3)
				{
					b.set_type(block_type::royal);
					royal_area += b.area;
					break;
				}
			}
		}
		if (b.get_type() == block_type::unknown && royal_area > 0 && !royal_found)
		{
			for (auto p : b.figure)
			{
				for (auto block_near : p->blocks_nearby)
				{
					if (block_near == block_type::royal)
					{
						b.set_type(block_type::royal);
						royal_area += b.area;
						break;
					}
				}
				if (b.get_type() == block_type::royal)
				{
					break;
				}
			}
		}
		if (b.get_type() == block_type::royal && royal_area > 1000)
		{
			royal_found = true;
		}
		if (b.get_type() == block_type::unknown)
		{
			bool is_near_royal = false;
			bool is_near_dock = false;
			// bool is_near_slums = false;
			// bool is_near_residential = false;
			for (auto p : b.figure)
			{
				if (p->blocks_nearby.Contains(block_type::royal))
				{
					is_near_royal = true;
				}
				if (p->blocks_nearby.Contains(block_type::dock))
				{
					is_near_dock = true;
				}

				// if (p->blocks_nearby.Contains(block_type::slums))
				// {
				// 	is_near_slums = true;
				// }
				// if (p->blocks_nearby.Contains(block_type::residential))
				// {
				// 	is_near_residential = true;
				// }
			}
			if (is_near_royal && !is_near_dock && b.area > 6000)
			{
				b.set_type(block_type::luxury);
			}
		}
	}
	int blocks_count = blocks.Num();
	int named_blocks = 0;
	int old_named_blocks;
	do
	{
		old_named_blocks = named_blocks;
		named_blocks = 0;
		for (auto& b : blocks)
		{
			if (b.get_type() != block_type::unknown)
			{
				named_blocks++;
			}
			else
			{
				TMap<block_type, int32> ElementCount;
				for (auto& fig : b.figure)
				{
					for (auto block : fig->blocks_nearby)
					{
						ElementCount.FindOrAdd(block)++;
					}
				}
				int blocks_near = 0;
				for (auto el : ElementCount)
				{
					blocks_near += el.Value;
				}
				if (ElementCount.IsEmpty())
				{
					continue;
				}
				int royal_count = 0;
				int dock_count = 0;
				int luxury_count = 0;
				int residential_count = 0;
				int slums_count = 0;
				for (auto el : ElementCount)
				{
					switch (el.Key)
					{
					case block_type::dock:
						{
							dock_count = el.Value;
							break;
						}
					case block_type::royal:
						{
							royal_count = el.Value;
							break;
						}
					case block_type::luxury:
						{
							luxury_count = el.Value;
							break;
						}
					case block_type::residential:
						{
							residential_count = el.Value;
							break;
						}
					case block_type::slums:
						{
							slums_count = el.Value;
							break;
						}
					default:
						break;
					}
				}
				if (blocks_near == 0)
				{
					continue;
				}
				int koeff = (dock_count * (-4) + royal_count * 6 + luxury_count * 3 + slums_count * (-8) +
							 residential_count * 2) /
					blocks_near;

				if (koeff <= -7 && luxury_count == 0 && royal_count == 0)
				{
					b.set_type(block_type::slums);
				}
				else if (koeff >= 0 && koeff < 4)
				{
					b.set_type(block_type::residential);
				}
				else if (koeff >= 4 && slums_count != 0 && dock_count != 0)
				{
					b.set_type(block_type::luxury);
				}
				else
				{
					int rand_val = rand() % 100;
					if (rand_val > 85 && slums_count != 0 && dock_count != 0)
					{
						b.set_type(block_type::luxury);
					}
					else if (rand_val > 50)
					{
						b.set_type(block_type::residential);
					}
					else if (luxury_count == 0 && royal_count == 0)
					{
						b.set_type(block_type::slums);
					}
				}
			}
		}
	}
	while (named_blocks < blocks_count && old_named_blocks != named_blocks);
}
void TerrainGen::process_houses(District& block)
{
	FVector block_center(0, 0, 0);
	for (auto p : block.figure)
	{
		block_center += p->point;
	}
	block_center /= block.self_figure.Num();
	if (block.get_type() == block_type::luxury)
	{
		if (!block.is_point_in_self_figure(block_center))
		{
			return;
		}
		for (int i = 1; i < block.self_figure.Num(); i++)
		{
			if (block.self_figure[i - 1].blocks_nearby.Contains(block_type::royal) &&
				block.self_figure[i].blocks_nearby.Contains(block_type::royal))
			{
				FVector point1 = AllGeometry::create_segment_at_angle(block.self_figure[i - 1].point,
																	  block.self_figure[i].point, block_center, 0, 40);
				FVector point2 = AllGeometry::create_segment_at_angle(
					block.self_figure[i - 1].point, block.self_figure[i].point, block_center, 180, 40);
				TArray<FVector> figure{point1, point2};
				if (block.create_house(figure, 40, 30))
				{
					break;
				}
			}
		}
	}
	else if (block.get_type() == block_type::residential)
	{
		int self_figure_count = block.self_figure.Num();
		for (int i = 1; i <= self_figure_count; i++)
		{
			FVector point1 = block.self_figure[i - 1].point;
			FVector point2 = block.self_figure[i % self_figure_count].point;
			double dist = FVector::Distance(point1, point2);
			double general_width = 0;
			while (general_width < dist)
			{
				double width = rand() % 3 + 5;
				if (rand() % 7 > 1)
				{
					double length = rand() % 5 + 10;
					double height = (rand() % 2 + 1) * 4;
					FVector point_beg = AllGeometry::create_segment_at_angle(
						point1, point2, (point2 - point1) / dist * (general_width + (width / 2)) + point1, 90, 1);
					FVector point_end = AllGeometry::create_segment_at_angle(point1, point2, point_beg, 90, length);
					TArray<FVector> figure{point_beg, point_end};
					block.create_house(figure, width, height);
				}
				general_width += (width + 1);
			}
		}
	}
}
