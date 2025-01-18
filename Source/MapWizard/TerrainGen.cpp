#include "TerrainGen.h"

void TerrainGen::add_conn(const TSharedPtr<Node>& node1, const TSharedPtr<Node>& node2)
{
	if (FVector::Distance(node1->get_FVector(), node2->get_FVector()) < 0.01)
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
	if (node2_to_insert->get_type() == node1_to_insert->get_type())
	{
		new_node->set_type(node2_to_insert->get_type());
	}
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

void TerrainGen::create_terrain(TArray<TSharedPtr<Node>>& roads_, TArray<District>& figures_array_, TArray<Street>& streets_array_,
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
						(FMath::FRand() * (static_cast<int>(av_size / point_row_counter / 2))) -
						(av_size / point_row_counter / 4)),
					static_cast<int>(av_size / point_row_counter * y) +
					(FMath::FRand() * (static_cast<int>(av_size / point_row_counter / 2))) -
					(av_size / point_row_counter / 4),
					0),
				(FMath::FRand() * static_cast<int>(av_size / point_row_counter / 2)) + (av_size / point_row_counter / 3)
			});
		}
	}

	double StartTime1 = FPlatformTime::Seconds();
	if (draw_stage >= EDrawStage::river)
	{
		if (water_type == EWaterType::river)
		{
			create_guiding_rivers();
		}
	}
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
	bridges.Empty();

	double EndTime3 = FPlatformTime::Seconds();
	double StartTime4 = FPlatformTime::Seconds();
	if (draw_stage >= EDrawStage::create_guiding_roads)
	{
		create_guiding_roads();
	}
	double EndTime4 = FPlatformTime::Seconds();
	double StartTime5 = FPlatformTime::Seconds();
	// for (int iter = 0; iter < 10; iter++)
	// {
	// 	for (auto& r : roads)
	// 	{
	// 		r->set_used(false);
	// 	}
	//
	// 	// for (auto& road_center : road_centers)
	// 	// {
	// 	// 	road_center->set_used(true);
	// 	// }
	//
	// 	for (auto& r : roads)
	// 	{
	// 		move_road(r);
	// 	}
	// }
	double EndTime5 = FPlatformTime::Seconds();
	double StartTime6 = FPlatformTime::Seconds();
	int old_nodes = 0;
	
	if (draw_stage >= EDrawStage::create_usual_roads)
	{
		while (roads.Num() != old_nodes)
		{
			old_nodes = roads.Num();
			create_usual_roads();
			if (roads.Num() > 2500)
			{
				break;
			}
		}
	}
	double EndTime6 = FPlatformTime::Seconds();
	double StartTime7 = FPlatformTime::Seconds();
	double EndTime7 = FPlatformTime::Seconds();

	double StartTime8 = FPlatformTime::Seconds();

	if (draw_stage >= EDrawStage::shrink_roads)
	{
		shrink_roads();
	}

	for (int i = 0; i < 2; i++)
	{
		for (auto& r : roads)
		{
			r->set_used(false);
		}
		//
		// for (auto& road_center : road_centers)
		// {
		// 	road_center->set_used(true);
		// }
		for (auto& r : roads)
		{
			move_road(r);
		}
	}
	
	if (central_node.IsValid())
	{
		auto central_point = central_node->get_FVector();
		// central_node = nullptr;
		create_circle(central_point, 200, royal, main_road, 25);
	}

	double EndTime8 = FPlatformTime::Seconds();
	double StartTime9 = FPlatformTime::Seconds();


	process_streets(roads, ways_array, main_road, false);

	
	process_streets(roads, ways_array, wall, true);
	double tower_radius = 20;
	TArray<FVector> towers;
	for (auto& s : ways_array)
	{
		if (s.type == wall)
		{
			bool is_ok = true;
			for (auto t : towers)
			{
				if (FVector::Distance(t, s.points[0]->point) < tower_radius * 2)
				{
					is_ok = false;
					break;
				}
			}
			if (is_ok)
			{
				towers.Add(s.points[0]->point);
			}
			is_ok = true;
			for (auto t : towers)
			{
				if (FVector::Distance(t, s.points[0]->point) < tower_radius * 2)
				{
					is_ok = false;
					break;
				}
			}
			if (is_ok)
			{
				towers.Add(s.points.Last()->point);
			}
		}
	}
	ways_array.RemoveAll([](Way& way) { return way.type == wall; });
	// roads.Empty();
	for (auto r : roads)
	{
		for (auto& rconn : r->conn)
		{
			rconn->in_street = false;
		}
	}
	for (auto t : towers)
	{
		create_circle(t, tower_radius, tower, wall, 8);
	}
	process_streets(roads, ways_array, wall, true);
	process_streets(roads, ways_array, {}, false);
	
	get_closed_figures(roads, figures_array, 200);
	
	double EndTime9 = FPlatformTime::Seconds();
	double StartTime10 = FPlatformTime::Seconds();

	if (draw_stage >= EDrawStage::process_districts)
	{
		process_districts(figures_array);
	}
	double EndTime10 = FPlatformTime::Seconds();


	TArray<District> district_roads;
	for (auto s : ways_array)
	{
		TArray<FVector> street_fvectors;
		for (auto& t : s.points)
		{
			street_fvectors.Add(t->point);
		}
		auto fig = AllGeometry::line_to_polygon(street_fvectors, 10, 10);
		Street road(fig);
		road.type = s.type;
		streets_array.Add(road);
	}
	
	for (int i = 0; i < roads.Num(); i++)
	{
		if (roads[i]->conn.Num() != roads[i]->get_point()->districts_nearby.Num())
		{
			FVector def_point = roads[i]->get_FVector();
			debug_points_array_.Add(def_point);
		}
	}

	double StartTime11 = FPlatformTime::Seconds();
	if (draw_stage >= EDrawStage::process_houses)
	{
		for (auto& b : figures_array)
		{
			process_houses(b);
		}
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
	streets_array_ = streets_array;
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
		-90 + (FMath::FRand() * 20), (FMath::FRand() * 20 + 10) * (av_river_length));

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
			start_point->get_FVector(), end_point->get_FVector(), end_point->get_FVector(), -60 + (FMath::FRand() * 120),
			(FMath::FRand() * 20 + 10) * (av_river_length)));
		create_guiding_river_segment(end_point, MakeShared<Node>(next_segment), old_node_left, old_node_right);
		if (FMath::FRand() * 4 >= 3)
		{
			did_river_multiplied = true;
			auto next_segment1 = MakeShared<Node>(Node(AllGeometry::create_segment_at_angle(
				start_point->get_FVector(), end_point->get_FVector(), end_point->get_FVector(), -60 + (FMath::FRand() * 120),
				(FMath::FRand() * 20 + 10) * (av_river_length))));
			create_guiding_river_segment(end_point, next_segment1, old_node_left, old_node_right);
		}
		if (FMath::FRand() * 8 >= 7)
		{
			did_river_multiplied = true;
			auto next_segment2 = MakeShared<Node>(Node(AllGeometry::create_segment_at_angle(
				start_point->get_FVector(), end_point->get_FVector(), end_point->get_FVector(), -60 + (FMath::FRand() * 120),
				(FMath::FRand() * 20 + 10) * (av_river_length))));
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
			return FMath::FRand() * 5 >= 4 ||
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
			(FMath::FRand() * (static_cast<int>(x_size / point_row_counter / 2))) - (x_size / point_row_counter / 4);
			auto y_val = y_size / point_row_counter * y +
			(FMath::FRand() * (static_cast<int>(x_size / point_row_counter / 2))) - (x_size / point_row_counter / 4);
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
		{
			return A.Get<0>() < B.Get<0>();
		});
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
				success_roads += create_guiding_road_segment(road_centers[i], local_road[j], true, point_type::main_road);
			}
		}
	}
	else if (city_plan == ECityPlan::rectangular)
	{
		max_road_length = av_road_length;
		min_road_length = av_road_length;
		road_forward_chance = 100;
		road_left_chance = 100;
		road_right_chance = 100;
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
				success_roads += create_guiding_road_segment(road_centers[i], local_road[j], true, point_type::main_road);
			}
		}
	}
	else if (city_plan == ECityPlan::radial || city_plan == ECityPlan::radial_circle)
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
		central_node = closest_center;
		roads.Empty();
		roads.AddUnique(closest_center);
		road_centers.Empty();
		road_centers.Add(closest_center);
		bridges.Empty();

		auto left_node = MakeShared<Node>(closest_center->get_FVector());
		left_node->set_FVector(FVector(closest_center->get_FVector().X, 0, 0));
		auto trade_path_angle = FMath::FRand() * 180;
		auto trade_path1 = AllGeometry::create_segment_at_angle(
			left_node->get_FVector(), closest_center->get_FVector(), closest_center->get_FVector(),
			trade_path_angle, 5000);
		auto trade_path2 = AllGeometry::create_segment_at_angle(
			left_node->get_FVector(), closest_center->get_FVector(), closest_center->get_FVector(),
			trade_path_angle + 180, 5000);
		FVector trade_path1_end;
		FVector trade_path2_end;
		auto trade_path1_end_point = AllGeometry::is_intersect_array(closest_center->get_FVector(), trade_path1, map_borders_array, false);
		auto trade_path2_end_point = AllGeometry::is_intersect_array(closest_center->get_FVector(), trade_path2, map_borders_array, false);
		if (trade_path1_end_point.IsSet() && trade_path2_end_point.IsSet())
		{
			trade_path1_end = trade_path1_end_point->Key;
			trade_path1_end = AllGeometry::create_segment_at_angle(
				closest_center->get_FVector(), trade_path1_end, closest_center->get_FVector(),
				0, FVector::Distance(closest_center->get_FVector(), trade_path1_end) - 0.01f);
			trade_path2_end = trade_path2_end_point->Key;
			trade_path2_end = AllGeometry::create_segment_at_angle(
				closest_center->get_FVector(), trade_path2_end, closest_center->get_FVector(),
				0, FVector::Distance(closest_center->get_FVector(), trade_path2_end) - 0.01f);
		}
		else return;

		auto trade_path1_end_node = MakeShared<Node>(trade_path1_end);
		auto trade_path2_end_node = MakeShared<Node>(trade_path2_end);
		trade_path1_end_node->unshrinkable = true;
		trade_path2_end_node->unshrinkable = true;

		create_guiding_road_segment(closest_center, trade_path1_end_node, true, point_type::main_road);
		create_guiding_road_segment(closest_center, trade_path2_end_node, true, point_type::main_road);

		TArray<TSharedPtr<Node>> circle_nodes;
		for (int angle = trade_path_angle + FMath::FRand() * 10 + 20; angle < trade_path_angle + 180 - 20; angle += FMath::FRand() * 10 + 20)
		{
			double dist_from_center = av_distance * (FMath::FRand() * 0.5 + 0.25);
			auto new_node_fvec = AllGeometry::create_segment_at_angle(
				left_node->get_FVector(), closest_center->get_FVector(), closest_center->get_FVector(),
				angle, dist_from_center);

			if (river_figure.is_point_in_figure(new_node_fvec))continue;
			TSharedPtr<Node> new_node(MakeShared<Node>(new_node_fvec));
			new_node->set_type(point_type::main_road);
			roads.Add(new_node);
			if (create_guiding_road_segment(closest_center, new_node, true, point_type::main_road))
			{
				road_centers.Add(new_node);
			}
			if (city_plan == ECityPlan::radial_circle)
			{
				auto new_circle_node_fvec = AllGeometry::create_segment_at_angle(left_node->get_FVector(), closest_center->get_FVector(), closest_center->get_FVector(),
					angle, dist_from_center + av_road_length);
				soft_borders_array.Add(new_node->get_FVector());
				if (river_figure.is_point_in_figure(new_circle_node_fvec))continue;
				TSharedPtr<Node> new_wall_node(MakeShared<Node>(new_circle_node_fvec));
				new_wall_node->set_type(point_type::wall);
				roads.Add(new_wall_node);
				circle_nodes.Add(new_wall_node);
			}
		}
		for (int angle = trade_path_angle + 180 + FMath::FRand() * 10 + 20; angle < trade_path_angle + 360 - 20; angle += FMath::FRand() * 10 + 20)
		{
			double dist_from_center = av_distance * (FMath::FRand() * 0.5 + 0.25);
			auto new_node_fvec = AllGeometry::create_segment_at_angle(
				left_node->get_FVector(), closest_center->get_FVector(), closest_center->get_FVector(),
				angle, dist_from_center);

			if (river_figure.is_point_in_figure(new_node_fvec))continue;
			TSharedPtr<Node> new_node(MakeShared<Node>(new_node_fvec));
			new_node->set_type(point_type::main_road);
			roads.Add(new_node);
			if (create_guiding_road_segment(closest_center, new_node, true, point_type::main_road))
			{
				road_centers.Add(new_node);
			}
			if (city_plan == ECityPlan::radial_circle)
			{
				auto new_circle_node_fvec = AllGeometry::create_segment_at_angle(left_node->get_FVector(), closest_center->get_FVector(), closest_center->get_FVector(),
					angle, dist_from_center + av_road_length);
				soft_borders_array.Add(new_node->get_FVector());
				if (river_figure.is_point_in_figure(new_circle_node_fvec))continue;
				TSharedPtr<Node> new_wall_node(MakeShared<Node>(new_circle_node_fvec));
				new_wall_node->set_type(point_type::wall);
				roads.Add(new_wall_node);
				circle_nodes.Add(new_wall_node);
			}
		}

		for (int i = 1; i <= circle_nodes.Num(); i++)
		{
			create_guiding_road_segment(circle_nodes[i - 1], circle_nodes[i % circle_nodes.Num()], true, point_type::wall);
		}
		TArray<TSharedPtr<Node>> local_road;
		for (auto bridge_point : bridges)
		{
			local_road.AddUnique(bridge_point.Key);
			local_road.AddUnique(bridge_point.Value);
		}
		int success_roads = 0;
		for (int i = 0; i < local_road.Num() && success_roads <= 1; i++)
		{
			success_roads = 0;
			local_road.Sort(
				[i, this, local_road](const TSharedPtr<Node>& Item1, const TSharedPtr<Node>& Item2)
				{
					return FVector::Distance(local_road[i]->get_FVector(), Item1->get_FVector()) <
					FVector::Distance(local_road[i]->get_FVector(), Item2->get_FVector());
				});
			for (int j = 0; j < local_road.Num(); j++)
			{
				success_roads += create_guiding_road_segment(local_road[i], local_road[j], false, point_type::main_road);
				if (success_roads <= 1) break;
			}
		}
	}
}

bool TerrainGen::create_guiding_road_segment(const TSharedPtr<Node>& start_point, const TSharedPtr<Node>& end_point, bool is_through_river, point_type road_type)
{
	if (AllGeometry::is_intersect_array(start_point, end_point, river, true).IsSet() && !is_through_river)
	{
		return false;
	}
	if (FVector::Distance(start_point->get_FVector(), end_point->get_FVector()) < 0.01)
	{
		return false;
	}
	TArray<TSharedPtr<Node>> road_segment_nodes;
	TSharedPtr<Node> local_start_point = start_point;
	TSharedPtr<Node> local_end_point = end_point;
	TSharedPtr<Node> next_start_point = start_point;
	int tries = 0;
	do
	{
		tries++;
		local_start_point = next_start_point;
		local_end_point = end_point;

		auto inter_point = AllGeometry::is_intersect_array(local_start_point, end_point, river, false);
		if (inter_point.IsSet())
		{
			int angle = -90;
			auto calc_angle = AllGeometry::calculate_angle_clock(inter_point->Value.Key->get_FVector(),
				inter_point->Key, local_start_point->get_FVector());
			if (calc_angle < 180)
				angle = 90;
			local_end_point = MakeShared<Node>(inter_point->Key);
			local_end_point->set_type(road_type);
			roads.AddUnique(local_end_point);
			auto point1 = inter_point->Value.Key;
			auto point2 = inter_point->Value.Value;
			auto point_end = AllGeometry::create_segment_at_angle(point1->get_FVector(), point2->get_FVector(),
				inter_point->Key, angle, 500);

			auto point_beg = AllGeometry::create_segment_at_angle(point1->get_FVector(), point2->get_FVector(),
				inter_point->Key, angle, 1);
			auto inter_point2 = AllGeometry::is_intersect_array(point_beg, point_end, river, false);
			if (inter_point2.IsSet())
			{
				auto new_node = MakeShared<Node>(inter_point2->Key);
				new_node->set_type(road_type);
				roads.AddUnique(new_node);
				create_segment(roads, local_end_point, new_node, true, road_type, TNumericLimits<double>::Max());
				next_start_point = new_node;

				FVector real_bridge_end = AllGeometry::create_segment_at_angle(local_end_point->get_FVector(), next_start_point->get_FVector(),
					next_start_point->get_FVector(), 0, 6);
				FVector real_bridge_start = AllGeometry::create_segment_at_angle(next_start_point->get_FVector(), local_end_point->get_FVector(),
					local_end_point->get_FVector(), 0, 6);
				local_end_point->set_FVector(real_bridge_start);
				next_start_point->set_FVector(real_bridge_end);
				if (road_type != point_type::wall)
				{
					bridges.Add(TTuple<TSharedPtr<Node>, TSharedPtr<Node>>(local_end_point, next_start_point));
				}
			}
			else return false;
		}

		TSharedPtr<Node> old_node = local_start_point;
		int dist_times = FVector::Distance(local_start_point->get_FVector(), local_end_point->get_FVector()) / (av_road_length);
		dist_times = dist_times < 1 ? 1 : dist_times;
		// UE_LOG(LogTemp, Warning, TEXT("количество раз %d"), dist_times);
		for (int i = 1; i <= dist_times; i++)
		{
			TSharedPtr<Node> node = MakeShared<Node>(0, 0, 0);
			node->set_type(road_type);
			node->set_FVector(local_start_point->get_FVector() +
				((local_end_point->get_FVector() - local_start_point->get_FVector()) / dist_times * i));
			if (i == dist_times)
			{
				node = local_end_point;
			}
			roads.AddUnique(node);
			create_segment(roads, old_node, node, true, road_type, max_road_length);
			old_node = node;
		}
	}
	while (FVector::Distance(local_end_point->get_FVector(), end_point->get_FVector()) > 0.01 && tries <= 10);
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
				if (node->unshrinkable) return false;
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
				if (FMath::FRand() * 100 <= road_forward_chance)
				{
					auto length = FVector::Distance(road_node->get_FVector(), road_node->conn[0]->node->get_FVector()) +
					(FMath::FRand() * 40) - 20;
					if (length < min_new_road_length)
					{
						length = min_new_road_length;
					}
					if (length > max_road_length)
					{
						length = max_road_length;
					}
					double angle_in_degrees = (FMath::FRand() * 10) - 5;
					auto line1 = AllGeometry::create_segment_at_angle(
						road_node->conn[0]->node->get_FVector(), road_node->get_FVector(), road_node->get_FVector(),
						angle_in_degrees, length);

					TSharedPtr<Node> new_node = MakeShared<Node>(line1);
					new_node->set_type(point_type::road);
					bool is_possible = false;

					if (!soft_borders_array.IsEmpty())
					{
						if (AllGeometry::is_point_in_figure(line1, soft_borders_array))
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
				if (FMath::FRand() * 100 <= road_left_chance)
				{
					auto length = FVector::Distance(road_node->get_FVector(), road_node->conn[0]->node->get_FVector()) +
					(FMath::FRand() * 40) - 20;
					if (length < min_new_road_length)
					{
						length = min_new_road_length;
					}
					if (length > max_road_length)
					{
						length = max_road_length;
					}
					double angle_in_degrees = 90 + (FMath::FRand() * 10) - 5;
					auto line2 = AllGeometry::create_segment_at_angle(
						road_node->conn[0]->node->get_FVector(), road_node->get_FVector(), road_node->get_FVector(),
						angle_in_degrees, length);
					TSharedPtr<Node> new_node2 = MakeShared<Node>(line2);
					new_node2->set_type(point_type::road);
					create_segment(add_road, road_node, new_node2, false, point_type::road, max_road_length);
				}
				if (FMath::FRand() * 100 <= road_right_chance)
				{
					auto length = FVector::Distance(road_node->get_FVector(), road_node->conn[0]->node->get_FVector()) +
					(FMath::FRand() * 40) - 20;
					if (length < min_new_road_length)
					{
						length = min_new_road_length;
					}
					if (length > max_road_length)
					{
						length = max_road_length;
					}
					double angle_in_degrees = -90 + (FMath::FRand() * 10) - 5;
					auto line3 = AllGeometry::create_segment_at_angle(
						road_node->conn[0]->node->get_FVector(), road_node->get_FVector(), road_node->get_FVector(),
						angle_in_degrees, length);
					TSharedPtr<Node> new_node3 = MakeShared<Node>(line3);
					new_node3->set_type(point_type::road);
					create_segment(add_road, road_node, new_node3, false, point_type::road, max_road_length);
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
			}
		}
	}
}

TOptional<TSharedPtr<Node>> TerrainGen::create_segment(TArray<TSharedPtr<Node>>& array, TSharedPtr<Node> start_point,
                                                       TSharedPtr<Node> end_point, bool to_exect_point, point_type type,
                                                       double max_length) const
{
	if (max_length == TNumericLimits<double>::Max())
	{
		add_conn(start_point, end_point);
		array.AddUnique(end_point);
		return end_point;
	}

	// if (!soft_borders_array.IsEmpty())
	// {
	// 	if (!AllGeometry::is_point_in_figure(end_point->get_FVector(), soft_borders_array))
	// 		return TOptional<TSharedPtr<Node>>();
	// }

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
			if (intersection.GetValue()->get_type() == wall && type != wall)
			{
				return TSharedPtr<Node>();
			}
			end_point = intersection.GetValue();
			end_point->set_type(type);
			tries++;
			if (tries > 10 || FVector::Distance(start_point->get_FVector(), end_point->get_FVector()) > max_length)
			{
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
				end_point->set_type(type);
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
	while (FVector::Distance(end_point->get_FVector(), backup_endpoint->get_FVector()) > 0.01);

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

void TerrainGen::get_closed_figures(TArray<TSharedPtr<Node>> nodes, TArray<District>& fig_array, int figure_threshold)
{
	bool is_river = false;
	if (nodes.Num() > 0 && nodes[0]->get_type() == point_type::river)
	{
		is_river = true;
	}
	for (auto node : nodes)
	{
		for (auto lconn : node->conn)
		{
			if (!lconn->figure->IsEmpty() || lconn->not_in_figure)
			{
				continue;
			}
			TSharedPtr<TArray<TSharedPtr<Point>>> figure_array = MakeShared<TArray<TSharedPtr<Point>>>();
			TArray<TSharedPtr<Conn>> conn_array;
			conn_array.Add(lconn);
			auto first_node = node;

			// first_node->print_connections();
			auto second_node = lconn->node;
			// second_node->print_connections();
			figure_array->Add(node->get_point());
			figure_array->Add(lconn->node->get_point());
			TSharedPtr<Node> rightest_node;
			TSharedPtr<Conn> this_conn;
			bool some_error = false;
			bool not_in_figure = false;
			double general_angle = 0;
			// while (!figure_array->Contains(second_node))
			while (second_node->get_FVector() != node->get_point()->point)
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
					conn->node->in_figure = true;
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
	{
		return Item1.figure.Num() > Item2.figure.Num();
	});

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
void TerrainGen::process_districts(TArray<District>& districts)
{
	districts.RemoveAll([this](District& Item1) { return !(Item1.shrink_size(Item1.self_figure, 3.0f, 6.0)); });

	districts.Sort([this](const District& Item1, const District& Item2) { return Item1.area > Item2.area; });
	double royal_area = 0;
	bool royal_found = false;

	for (auto& c : custom_districts)
	{
		for (auto& d : districts)
		{
			if (d.is_point_in_self_figure(c.Key))
			{
				if (c.Value == district_type::royal)
				{
					royal_found = true;
				}
				d.set_type(c.Value);
				break;
			}
		}
	}

	// bool dock_found = false;
	for (auto& b : districts)
	{
		bool is_river = false;
		for (auto r : river)
		{
			if (b.is_point_in_self_figure(r->get_FVector()))
			{
				is_river = true;
				b.set_type(district_type::dock);
				break;
			}
		}
		if (b.get_type() == district_type::unknown && !royal_found && royal_area == 0)
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
					b.set_type(district_type::royal);
					royal_area += b.area;
					break;
				}
			}
		}
		if (b.get_type() == district_type::unknown && royal_area > 0 && !royal_found)
		{
			for (auto p : b.figure)
			{
				for (auto district_near : p->districts_nearby)
				{
					if (district_near == district_type::royal)
					{
						b.set_type(district_type::royal);
						royal_area += b.area;
						break;
					}
				}
				if (b.get_type() == district_type::royal)
				{
					break;
				}
			}
		}
		if (b.get_type() == district_type::royal && royal_area > 1000)
		{
			royal_found = true;
		}
		if (b.get_type() == district_type::unknown)
		{
			bool is_near_royal = false;
			bool is_near_dock = false;
			// bool is_near_slums = false;
			// bool is_near_residential = false;
			for (auto p : b.figure)
			{
				if (p->districts_nearby.Contains(district_type::royal))
				{
					is_near_royal = true;
				}
				if (p->districts_nearby.Contains(district_type::dock))
				{
					is_near_dock = true;
				}
			}
			if (is_near_royal && !is_near_dock && b.area > 6000)
			{
				b.set_type(district_type::luxury);
			}
		}
	}
	int districts_count = districts.Num();
	int named_districts = 0;
	int old_named_districts;
	do
	{
		old_named_districts = named_districts;
		named_districts = 0;
		for (auto& b : districts)
		{
			if (b.get_type() != district_type::unknown)
			{
				named_districts++;
			}
			else
			{
				TMap<district_type, int32> ElementCount;
				for (auto& fig : b.figure)
				{
					for (auto district : fig->districts_nearby)
					{
						ElementCount.FindOrAdd(district)++;
					}
				}
				int districts_near = 0;
				for (auto el : ElementCount)
				{
					districts_near += el.Value;
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
					case district_type::dock:
						{
							dock_count = el.Value;
							break;
						}
					case district_type::royal:
						{
							royal_count = el.Value;
							break;
						}
					case district_type::luxury:
						{
							luxury_count = el.Value;
							break;
						}
					case district_type::residential:
						{
							residential_count = el.Value;
							break;
						}
					case district_type::slums:
						{
							slums_count = el.Value;
							break;
						}
					default:
						break;
					}
				}
				if (districts_near == 0)
				{
					continue;
				}
				int koeff = (dock_count * (-4) + royal_count * 6 + luxury_count * 3 + slums_count * (-8) +
					residential_count * 2) /
				districts_near;

				if (koeff <= -7 && luxury_count == 0 && royal_count == 0)
				{
					b.set_type(district_type::slums);
				}
				else if (koeff >= 0 && koeff < 4)
				{
					b.set_type(district_type::residential);
				}
				else if (koeff >= 4 && slums_count != 0 && dock_count != 0)
				{
					b.set_type(district_type::luxury);
				}
				else
				{
					int rand_val = FMath::FRand() * 100;
					if (rand_val > 85 && slums_count != 0 && dock_count != 0)
					{
						b.set_type(district_type::luxury);
					}
					else if (rand_val > 50)
					{
						b.set_type(district_type::residential);
					}
					else if (luxury_count == 0 && royal_count == 0)
					{
						b.set_type(district_type::slums);
					}
				}
			}
		}
	}
	while (named_districts < districts_count && old_named_districts != named_districts);
}
void TerrainGen::process_houses(District& district)
{
	FVector district_center(0, 0, 0);
	for (auto p : district.figure)
	{
		district_center += p->point;
	}
	district_center /= district.self_figure.Num();
	if (district.get_type() == district_type::luxury)
	{
		if (!district.is_point_in_self_figure(district_center))
		{
			return;
		}
		for (int i = 1; i < district.self_figure.Num(); i++)
		{
			if (district.self_figure[i - 1].districts_nearby.Contains(district_type::royal) &&
				district.self_figure[i].districts_nearby.Contains(district_type::royal))
			{
				FVector point1 = AllGeometry::create_segment_at_angle(district.self_figure[i - 1].point,
					district.self_figure[i].point, district_center, 0, 40);
				FVector point2 = AllGeometry::create_segment_at_angle(
					district.self_figure[i - 1].point, district.self_figure[i].point, district_center, 180, 40);
				TArray<FVector> figure{point1, point2};
				if (district.create_house(figure, 40, 30))
				{
					break;
				}
			}
		}
	}
	else if (district.get_type() == district_type::residential)
	{
		int self_figure_count = district.self_figure.Num();
		for (int i = 1; i <= self_figure_count; i++)
		{
			FVector point1 = district.self_figure[i - 1].point;
			FVector point2 = district.self_figure[i % self_figure_count].point;
			double dist = FVector::Distance(point1, point2);
			double general_width = 0;
			while (general_width < dist)
			{
				double width = FMath::FRand() * 3 + 5;
				if (FMath::FRand() * 7 > 1)
				{
					double length = FMath::FRand() * 5 + 10;
					double height = (FMath::FRand() * 2 + 1) * 4;
					FVector point_beg = AllGeometry::create_segment_at_angle(
						point1, point2, (point2 - point1) / dist * (general_width + (width / 2)) + point1, 90, 1);
					FVector point_end = AllGeometry::create_segment_at_angle(point1, point2, point_beg, 90, length);
					TArray<FVector> figure{point_beg, point_end};
					district.create_house(figure, width, height);
				}
				general_width += (width + 1);
			}
		}
	}
}
void TerrainGen::create_special_district(TArray<FVector>& figure, point_type type, FVector point)
{
	TArray<TSharedPtr<Node>> nodes;
	for (auto f : figure)
	{
		auto new_node = MakeShared<Node>(f);
		new_node->set_type(type);
		roads.AddUnique(new_node);
		nodes.AddUnique(new_node);
	}

	FVector center_point(0, 0, 0);
	for (auto f : figure)
	{
		center_point += f;
	}
	center_point /= figure.Num();
	for (int i = 1; i <= nodes.Num(); i++)
	{
		create_segment(roads, nodes[i - 1], nodes[i % nodes.Num()], true, type, 5000);
	}

	TArray<FVector> inner_figure;
	for (auto f : figure)
	{
		auto new_inner_point = AllGeometry::create_segment_at_angle(center_point, f, f, 180, 0.01);
		inner_figure.Add(new_inner_point);
	}
	
	roads.RemoveAll(
		[&, point](const TSharedPtr<Node>& node)
		{
			if (AllGeometry::is_point_in_figure(node->get_FVector(), inner_figure) || FVector::Distance(node->get_FVector(), point) < 0.1)
			{
				node->delete_me();
				return true;
			}
			return false;
		});
	// auto streets_double = streets_array;
	TArray<TSharedPtr<Point>> node_points;
	for (auto& p : nodes)
	{
		node_points.Add(p->get_point());
	}
	Way way(node_points);
	way.type = type;
	ways_array.Add(way);
}
void TerrainGen::create_circle(FVector point, double radius, district_type type, point_type road_type, int vertex_count)
{
	custom_districts.Add(TTuple<FVector, district_type>(point, type));
	FVector left_point = point;
	left_point.X = 0;
	TArray<FVector> figure;
	for (int i = 1; i <= vertex_count; i++)
	{
		figure.Add(AllGeometry::create_segment_at_angle(left_point, point, point, 360 / vertex_count * i, radius));
	}
	create_special_district(figure, road_type, point);
}
void TerrainGen::process_streets(TArray<TSharedPtr<Node>> nodes, TArray<Way>& fig_array, point_type type, bool is_persistent)
{
	point_type type_used;
	type_used = type != unidentified ? type : road;
	for (auto node : nodes)
	{
		for (auto nconn : node->conn)
		{
			if (nconn->in_street)
			{
				continue;
			}
			if (type != unidentified && (node->get_type() != type || nconn->node->get_type() != type))
			{
				continue;
			}

			auto street_points = MakeShared<TArray<TSharedPtr<Point>>>();

			street_points->Add(node->get_point());
			street_points->Add(nconn->node->get_point());

			auto first_point = node;
			auto second_point = nconn->node;
			TArray<TSharedPtr<Conn>> conn_array{};
			auto next_point = first_point->get_next_point(second_point->get_point());
			auto prev_point = first_point->get_prev_point(second_point->get_point());
			if (next_point.IsSet() && prev_point.IsSet())
			{
				if (next_point.GetValue()->in_street || prev_point.GetValue()->in_street)
				{
					continue;
				}
				conn_array.Add(next_point.GetValue());
				conn_array.Add(prev_point.GetValue());
			}
			else
			{
				continue;
			}
			TSharedPtr<Node> third_point;
			bool is_next_found;
			do
			{
				TMap<float, TSharedPtr<Node>> nodes_by_angle;
				for (auto second_node : second_point->conn)
				{
					if (is_persistent && second_node->node->get_type() != type_used)
					{
						continue;
					}
					auto current_angle = AllGeometry::calculate_angle(first_point->get_FVector(), second_point->get_FVector(), second_node->node->get_FVector(), false);
					if (current_angle > 179.99 && current_angle < 180.01) { continue; }
					auto angle = FMath::Abs(180 - current_angle);
					nodes_by_angle.Add(angle, second_node->node);
				}
				nodes_by_angle.KeySort([](float A, float B)
				{
					return A < B;
				});

				is_next_found = false;
				for (auto& n : nodes_by_angle)
				{
					if (nodes_by_angle.Num() > 2)
					{
						if (n.Key > 20)
						{
							break;
						}
					}
					third_point = n.Value;
					auto second_next = second_point->get_next_point(third_point->get_point());
					auto second_prev = second_point->get_prev_point(third_point->get_point());
					if (second_next.IsSet() && second_prev.IsSet())
					{
						if (second_next.GetValue()->in_street || second_prev.GetValue()->in_street)
						{
							is_next_found = false;
							continue;
						}
						second_next.GetValue()->in_street = true;
						second_prev.GetValue()->in_street = true;
						street_points->Add(third_point->get_point());
					}

					first_point = second_point;
					second_point = third_point;
					is_next_found = true;
					break;
				}
				// is_next_found = false;
				
			}
			while (is_next_found);
			first_point = nconn->node;
			second_point = node;
			third_point = node;
			do
			{
				TMap<float, TSharedPtr<Node>> nodes_by_angle;
				for (auto second_node : second_point->conn)
				{
					if (is_persistent && second_node->node->get_type() != type_used)
					{
						continue;
					}
					auto angle = FMath::Abs(180 - AllGeometry::calculate_angle(first_point->get_FVector(), second_point->get_FVector(), second_node->node->get_FVector(), false));
					nodes_by_angle.Add(angle, second_node->node);
				}
				nodes_by_angle.KeySort([](float A, float B)
				{
					return A < B;
				});

				is_next_found = false;
				for (auto& n : nodes_by_angle)
				{
					if (n.Key > 20)
					{
						break;
					}
					third_point = n.Value;
					auto second_next = second_point->get_next_point(third_point->get_point());
					auto second_prev = second_point->get_prev_point(third_point->get_point());
					if (second_next.IsSet() && second_prev.IsSet())
					{
						if (second_next.GetValue()->in_street || second_prev.GetValue()->in_street)
						{
							is_next_found = false;
							continue;
						}
						second_next.GetValue()->in_street = true;
						second_prev.GetValue()->in_street = true;
							street_points->Insert(third_point->get_point(), 0);
						}
					first_point = second_point;
					second_point = third_point;
					is_next_found = true;
					break;
				}
				// is_next_found = false;
				
			}
			while (is_next_found);
			for (auto conn : conn_array)
			{
				conn->in_street = true;
			}
			Way street(*street_points);
			street.type = type_used;
			fig_array.Add(street);
		}
	}
}
