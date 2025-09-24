#include "TerrainGen.h"

void TerrainGen::add_conn(const TSharedPtr<Node>& node1,
                          const TSharedPtr<Node>& node2)
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

TSharedPtr<Node> TerrainGen::insert_conn(
	const TSharedPtr<Node>& node1_to_insert,
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
	if (node2_to_insert->get_node_type() == node1_to_insert->get_node_type())
	{
		new_node->set_type(node2_to_insert->get_node_type());
	}
	add_conn(node1_to_insert, new_node);
	add_conn(node2_to_insert, new_node);
	return new_node;
}

void TerrainGen::move_river(const TSharedPtr<Node>& node1,
                            const TSharedPtr<Node>& node2, const TArray<WeightedPoint>& weighted_points,
                            TArray<TSharedPtr<Node>>& river)
{
	if (node1->is_used() || node2->is_used())
	{
		return;
	}

	FVector point1 = node1->get_FVector();
	FVector backup_point1 = node1->get_FVector();
	point_shift(point1, weighted_points);
	FVector point2 = node2->get_FVector();
	FVector backup_point2 = node2->get_FVector();
	point_shift(point2, weighted_points);

	for (auto conn : node1->conn)
	{
		double dist = FVector::Distance(conn->node->get_FVector(), point1);
		if (dist > map_params.max_river_length / 3 * 2 || dist < map_params.av_river_length / 3 * 2)
		{
			return;
		}
	}
	for (auto conn : node2->conn)
	{
		double dist = FVector::Distance(conn->node->get_FVector(), point2);
		if (dist > map_params.max_river_length || dist < map_params.av_river_length / 3 * 2)
		{
			return;
		}
	}
	if (
		// FVector::Distance(point1, point2) > map_params.max_river_length / 3 * 2 ||
		FVector::Distance(point1, point2) < map_params.av_river_length / 3 * 2)
	{
		return;
	}

	if (!AllGeometry::is_intersect_array(backup_point1, point1, river, false).
		IsSet() &&
		!AllGeometry::is_intersect_array(backup_point1, point1,
		                                 map_borders_array, true).IsSet())
	{
		node1->set_FVector(point1);
		// return;
	}
	if (!AllGeometry::is_intersect_array(backup_point2, point2, river, false).
		IsSet() &&
		!AllGeometry::is_intersect_array(backup_point2, point2,
		                                 map_borders_array, true).IsSet())
	{
		node2->set_FVector(point2);
		// return;
	}
}

void TerrainGen::move_road(const TSharedPtr<Node>& node, const TArray<WeightedPoint>& weighted_points,
                           const TArray<TSharedPtr<Node>>& river)
{
	if (node->is_used() || node->is_unmovable())
	{
		return;
	}

	FVector point = node->get_FVector();
	FVector backup_point = node->get_FVector();

	if (node->conn.Num() == 2)
	{
		if (!AllGeometry::is_intersect_array(node->conn[0]->node->get_FVector(),
		                                     node->conn[1]->node->get_FVector(),
		                                     river, true).IsSet() &&
			!AllGeometry::is_intersect_array(node->conn[0]->node->get_FVector(),
			                                 node->conn[1]->node->get_FVector(),
			                                 map_borders_array, true).IsSet() &&
			!AllGeometry::is_intersect_array(node->conn[0]->node->get_FVector(),
			                                 node->conn[1]->node->get_FVector(),
			                                 road_nodes, false).IsSet())
		{
			point = node->conn[0]->node->get_FVector() + node->conn[1]->node->get_FVector() / 2;
		}
	}

	point_shift(point, weighted_points);

	for (auto conn : node->conn)
	{
		if (FVector::Distance(conn->node->get_FVector(), point) >
			map_params.max_road_length)
		{
			return;
		}
	}


	if (!AllGeometry::is_intersect_array(backup_point, point, river, true).
		IsSet() &&
		!AllGeometry::is_intersect_array(backup_point, point, map_borders_array,
		                                 true).IsSet() &&
		!AllGeometry::is_intersect_array(backup_point, point, road_nodes, false).
		IsSet())
	{
		node->set_FVector(point);
		return;
	}
}

void TerrainGen::create_terrain(TArray<TSharedPtr<Node>>& roads_,
                                TArray<TSharedPtr<District>>& figures_array_,
                                TArray<TSharedPtr<Street>>& streets_array_,
                                TArray<TSharedPtr<Street>>& segments_array_,
                                TArray<TSharedPtr<District>>& river_figure_,
                                TArray<TSharedPtr<Node>>& map_borders_array_,
                                TArray<TSharedPtr<PointObject>>& point_objects_array_,
                                TArray<FVector>& debug_points_array_, TArray<FVector>& debug2_points_array_)
{
	// river.Empty();
	road_nodes.Empty();
	road_centers.Empty();
	map_points_array.Empty();
	map_borders_array.Empty();
	guididng_roads_array.Empty();
	// weighted_points.Empty();
	figures_array_.Empty();
	shapes_array.Empty();

	auto map_node1 = MakeShared<Node>(0, 0, 0);
	auto map_node2 = MakeShared<Node>(0, map_params.y_size, 0);
	auto map_node3 = MakeShared<Node>(map_params.x_size, map_params.y_size, 0);
	auto map_node4 = MakeShared<Node>(map_params.x_size, 0, 0);
	add_conn(map_node1, map_node2);
	add_conn(map_node2, map_node3);
	add_conn(map_node3, map_node4);
	add_conn(map_node4, map_node1);
	map_borders_array.Add(map_node4);
	map_borders_array.Add(map_node3);
	map_borders_array.Add(map_node2);
	map_borders_array.Add(map_node1);


	TArray<WeightedPoint> weighted_points{};
	create_weighted_points(weighted_points);
	TArray<TSharedPtr<Node>> river{};
	// if (draw_stage >= EDrawStage::river)
	// {
	create_rivers(weighted_points, river);
	road_nodes = river;


	if (map_params.draw_stage >= EDrawStage::create_guiding_roads)
	{
		create_guiding_roads(weighted_points, river);
	}
	for (int iter = 0; iter < 10; iter++)
	{
		for (auto& r : road_nodes)
		{
			r->set_used(false);
		}

		// for (auto& road_center : road_centers)
		// {
		// 	road_center->set_used(true);
		// }

		for (auto& r : road_nodes)
		{
			move_road(r, weighted_points, river);
		}
	}
	int old_nodes = 0;

	if (map_params.draw_stage >= EDrawStage::create_usual_roads)
	{
		while (road_nodes.Num() != old_nodes)
		{
			old_nodes = road_nodes.Num();
			create_usual_roads(weighted_points, river);
			if (road_nodes.Num() > 2500)
			{
				break;
			}
		}
	}
	for (int i = 0; i < 2; i++)
	{
		for (auto& r : road_nodes)
		{
			r->set_used(false);
		}
		//
		// for (auto& road_center : road_centers)
		// {
		// 	road_center->set_used(true);
		// }
		for (auto& r : road_nodes)
		{
			move_road(r, weighted_points, river);
		}
	}
	river.Empty();

	if (map_params.draw_stage >= EDrawStage::shrink_roads)
	{
		shrink_roads();
	}

	process_streets(road_nodes, streets_array, point_type::wall, true);
	double tower_radius = 20;
	TArray<FVector> towers;

	for (auto& s : streets_array)
	{
		bool is_ok = true;

		is_ok = true;
		for (auto t : towers)
		{
			if (FVector::Distance(t, s->street_vertices.Last()->get_FVector()) <
				tower_radius * 2)
			{
				is_ok = false;
				break;
			}
		}
		if (is_ok && !s->street_vertices.IsEmpty())
		{
			towers.Add(s->street_vertices.Last()->get_FVector());
		}
	}
	for (auto& s : streets_array)
	{
		bool is_ok = true;

		is_ok = true;
		for (auto t : towers)
		{
			if (FVector::Distance(t, s->street_vertices[0]->get_FVector()) <
				tower_radius * 2)
			{
				is_ok = false;
				break;
			}
		}
		if (is_ok && !s->street_vertices.IsEmpty())
		{
			towers.Add(s->street_vertices[0]->get_FVector());
		}
	}

	streets_array.Empty();
	// roads.Empty();
	for (auto r : road_nodes)
	{
		for (auto& rconn : r->conn)
		{
			rconn->in_street = false;
		}
	}

	if (central_node.IsValid())
	{
		auto central_point = central_node->get_FVector();
		// central_node = nullptr;
		create_circle_by_existing_nodes(central_point, 200, 40, district_type::royal,
		                                point_type::main_road, 50, true, true);
	}
	// drawing squares
	for (int i = 0; i < 15; i++)
	{
		FVector point(FMath::RandRange(0.0, map_params.x_size), FMath::RandRange(0.0, map_params.y_size), 0);
		if (is_point_in_river(point))
		{
			i--;
			continue;
		}
		create_circle_by_existing_nodes(point, 75, 75, district_type::tower,
		                                point_type::road, 8, true, true);
	}

	for (auto t : towers)
	{
		create_circle(t, tower_radius, district_type::tower, point_type::wall,
		              8);
	}
	process_streets(road_nodes, streets_array, point_type::wall, true);
	process_streets(road_nodes, streets_array, point_type::river, true);
	process_streets(road_nodes, streets_array, point_type::main_road, false);

	process_streets(road_nodes, streets_array, {}, false);

	segments_array = process_segments(streets_array);
	// shrink_roads();
	get_closed_figures(road_nodes, shapes_array, 200);
	if (map_params.draw_stage >= EDrawStage::process_districts)
	{
		process_districts(shapes_array);
	}

	for (auto s : streets_array)
	{
		TArray<FVector> street_fvectors;
		for (auto& t : s->street_vertices)
		{
			street_fvectors.Add(t->get_FVector());
		}
		double width;
		width = s->type == point_type::road ? map_params.road_width : map_params.main_road_width;
		s->street_vertexes = AllGeometry::line_to_polygon(
			street_fvectors, width);
	}

	for (auto s : segments_array)
	{
		TArray<FVector> street_fvectors;
		for (auto& t : s->street_vertices)
		{
			street_fvectors.Add(t->get_FVector());
		}
		double width;
		width = s->type == point_type::road ? map_params.road_width : map_params.main_road_width;
		s->street_vertexes = AllGeometry::line_to_polygon(
			street_fvectors, width);
	}


	process_lights();

	if (map_params.draw_stage >= EDrawStage::process_houses)
	{
		for (auto& b : shapes_array)
		{
			process_houses(b);
		}
	}

	segments_array_ = segments_array;
	figures_array_ = shapes_array;
	streets_array_ = streets_array;
	river_figure_ = river_figures;
	map_borders_array_ = map_borders_array;
	roads_ = road_nodes;
	point_objects_array_ = point_objects_array;

	debug2_points_array_ = debug2_points_array;
	debug_points_array_ = debug_points_array;
}

void TerrainGen::create_weighted_points(TArray<WeightedPoint>& weighted_points)
{
	double points_count = 64;
	double av_size = (map_params.x_size + map_params.y_size) / 2;
	weighted_points.Empty();

	const double point_row_counter = sqrt(points_count);
	for (double x = 1; x < point_row_counter; x++)
	{
		for (int y = 1; y < point_row_counter; y++)
		{
			weighted_points.Add(WeightedPoint{
				FVector(static_cast<int>(av_size / point_row_counter * x +
					        (FMath::FRand() * (static_cast<int>(av_size /
						        point_row_counter / 2))) -
					        (av_size / point_row_counter / 4)),
				        static_cast<int>(av_size / point_row_counter * y) +
				        (FMath::FRand() * (static_cast<int>(av_size /
					        point_row_counter / 2))) -
				        (av_size / point_row_counter / 4),
				        0),
				(FMath::FRand() * static_cast<int>(av_size / point_row_counter /
					2)) + (av_size / point_row_counter / 3)
			});
		}
	}
}

void TerrainGen::create_rivers(const TArray<WeightedPoint>& weighted_points, TArray<TSharedPtr<Node>>& river)
{
	double StartTime1 = FPlatformTime::Seconds();

	if (map_params.water_type == EWaterType::river)
	{
		create_guiding_rivers(river);
	}
	double EndTime1 = FPlatformTime::Seconds();
	double StartTime3 = FPlatformTime::Seconds();
	for (int iter = 0; iter < 100; iter++)
	{
		for (auto& b : bridges)
		{
			move_river(b.Key, b.Value, weighted_points, river);
		}
	}
	get_river_figure(river);
	river.Empty();
	for (auto& river_figure : river_figures)
	{
		for (auto r : river_figure->figure)
		{
			r->set_unmovable();
			r->set_unshrinkable();
			r->set_type(point_type::river);
			river.AddUnique(r);
		}
	}
	bridges.Empty();
}


void TerrainGen::create_guiding_rivers(TArray<TSharedPtr<Node>>& river)
{
	FVector start_river_point(0, 0, 0);
	FVector end_river_point(0, map_params.y_size, 0);
	auto start_point = MakeShared<Node>(
		(start_river_point + end_river_point) / 2);

	FVector end_river =
		AllGeometry::create_segment_at_angle(FVector(0, 0, 0),
		                                     FVector{0, map_params.y_size, 0}, start_point->get_FVector(),
		                                     -90 + (FMath::FRand() * 20),
		                                     (FMath::FRand() * 20 + 10) * (map_params.av_river_length));

	auto end_point = MakeShared<Node>(end_river);
	auto start_point_left = MakeShared<Node>(
		FVector(0, map_params.y_size / 2 - map_params.av_river_length / 2, 0));
	start_point_left->set_type(point_type::river);
	auto start_point_right = MakeShared<Node>(
		FVector(0, map_params.y_size / 2 + map_params.av_river_length / 2, 0));
	start_point_right->set_type(point_type::river);
	river.Add(start_point_left);
	river.Add(start_point_right);
	// guiding_river.Add(start_point);
	add_conn(start_point_left, start_point_right);
	create_guiding_river_segment(start_point, end_point, start_point_left,
	                             start_point_right, river);
}

void TerrainGen::create_guiding_river_segment(const TSharedPtr<Node>& start_point, const TSharedPtr<Node>& end_point,
                                              const TSharedPtr<Node>& start_point_left,
                                              const TSharedPtr<Node>& start_point_right,
                                              TArray<TSharedPtr<Node>>& river)
{
	guiding_river.AddUnique(start_point);
	bool is_ending = false;

	auto intersect_river = AllGeometry::is_intersect_array(
		start_point, end_point, guiding_river, false);
	auto intersect_border = AllGeometry::is_intersect_array(
		start_point, end_point, map_borders_array, false);
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
	TSharedPtr<Node> end_point_left = MakeShared<Node>(
		AllGeometry::create_segment_at_angle(
			start_point->get_FVector(), end_point->get_FVector(),
			end_point->get_FVector(), -90, map_params.av_river_length / 2));
	auto left_map_border_intersect = AllGeometry::is_intersect_array(
		end_point->get_FVector(), end_point_left->get_FVector(),
		map_borders_array, false);
	if (left_map_border_intersect.IsSet())
	{
		end_point_left->set_FVector(left_map_border_intersect->Key);
	}
	TSharedPtr<Node> end_point_right = MakeShared<Node>(
		AllGeometry::create_segment_at_angle(
			start_point->get_FVector(), end_point->get_FVector(),
			end_point->get_FVector(), 90, map_params.av_river_length / 2));
	auto right_map_border_intersect = AllGeometry::is_intersect_array(
		end_point->get_FVector(), end_point_right->get_FVector(),
		map_borders_array, false);
	if (right_map_border_intersect.IsSet())
	{
		end_point_right->set_FVector(right_map_border_intersect->Key);
	}
	if (right_map_border_intersect.IsSet() || left_map_border_intersect.IsSet())
	{
		is_ending = true;
	}
	end_point_left->set_type(point_type::river);
	end_point_right->set_type(point_type::river);
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
	int dist_times = FVector::Distance(start_point->get_FVector(),
	                                   end_point->get_FVector()) / (
		map_params.av_river_length);
	for (int i = 1; i <= dist_times; i++)
	{
		bridges.Add(MakeTuple(old_node_left, old_node_right));
		auto node_ptr = MakeShared<Node>();
		TSharedPtr<Node> node_ptr_left =
			MakeShared<Node>(start_point_left->get_FVector() +
				((end_point_left->get_FVector() - start_point_left->
					get_FVector()) / dist_times * i));
		TSharedPtr<Node> node_ptr_right =
			MakeShared<Node>(start_point_right->get_FVector() +
				((end_point_right->get_FVector() - start_point_right->
					get_FVector()) / dist_times * i));
		node_ptr_left->set_type(point_type::river);
		node_ptr_right->set_type(point_type::river);
		if (i != dist_times)
		{
			create_segment(river, node_ptr_left, old_node_left, true,
			               point_type::river, map_params.max_river_length);
			create_segment(river, node_ptr_right, old_node_right, true,
			               point_type::river, map_params.max_river_length);
		}
		else
		{
			node_ptr_left = end_point_left;
			node_ptr_right = end_point_right;
			create_segment(river, end_point_left, old_node_left, true,
			               point_type::river, map_params.max_river_length);
			create_segment(river, end_point_right, old_node_right, true,
			               point_type::river, map_params.max_river_length);
			// end_point->add_connection(old_node, false);
			// old_node->add_connection(end_point, false);
		}
		river.AddUnique(node_ptr_left);
		river.AddUnique(node_ptr_right);
		custom_distr_nodes.Add(
			CustomDistrNodes(node_ptr_left, node_ptr_right, old_node_right));
		// custom_districts.Add(TTuple<FVector, district_type>((
		// 	node_ptr_left->get_FVector() + node_ptr_right->get_FVector()
		// 	+ old_node_left->get_FVector() + old_node_right->get_FVector()) / 4, water));
		// create_segment(river, node_ptr_left, node_ptr_right, true, point_type::river, 5000);
		old_node_left = node_ptr_left;
		old_node_right = node_ptr_right;
	}

	if (!is_ending)
	{
		Node next_segment = Node(AllGeometry::create_segment_at_angle(
			start_point->get_FVector(), end_point->get_FVector(),
			end_point->get_FVector(), -60 + (FMath::FRand() * 120),
			(FMath::FRand() * 20 + 10) * (map_params.av_river_length)));
		create_guiding_river_segment(end_point, MakeShared<Node>(next_segment),
		                             old_node_left, old_node_right, river);
		if (FMath::FRand() * 4 >= 3)
		{
			did_river_multiplied = true;
			auto next_segment1 = MakeShared<Node>(Node(
				AllGeometry::create_segment_at_angle(
					start_point->get_FVector(), end_point->get_FVector(),
					end_point->get_FVector(), -60 + (FMath::FRand() * 120),
					(FMath::FRand() * 20 + 10) * (map_params.av_river_length))));
			create_guiding_river_segment(end_point, next_segment1,
			                             old_node_left, old_node_right, river);
		}
		if (FMath::FRand() * 8 >= 7)
		{
			did_river_multiplied = true;
			auto next_segment2 = MakeShared<Node>(Node(
				AllGeometry::create_segment_at_angle(
					start_point->get_FVector(), end_point->get_FVector(),
					end_point->get_FVector(), -60 + (FMath::FRand() * 120),
					(FMath::FRand() * 20 + 10) * (map_params.av_river_length))));
			create_guiding_river_segment(end_point, next_segment2,
			                             old_node_left, old_node_right, river);
		}
	}
	else
	{
		add_conn(old_node_left, old_node_right);
	}
}

bool TerrainGen::is_point_in_river(FVector point)
{
	for (auto river_district : river_figures)
	{
		if (river_district->is_point_in_figure(point))
		{
			return true;
		}
	}
	return false;
}


void TerrainGen::create_guiding_roads(TArray<WeightedPoint>& weighted_points, const TArray<TSharedPtr<Node>>& river)
{
	// double av_size = (x_size+map_params.y_size)/2;
	constexpr double point_row_counter = 9;
	for (double x = 1; x < point_row_counter; x++)
	{
		for (double y = 1; y < point_row_counter; y++)
		{
			auto x_val = map_params.x_size / point_row_counter * x +
			(FMath::FRand() * (static_cast<int>(map_params.x_size / point_row_counter /
				2))) - (map_params.x_size / point_row_counter / 4);
			auto y_val = map_params.y_size / point_row_counter * y +
			(FMath::FRand() * (static_cast<int>(map_params.x_size / point_row_counter /
				2))) - (map_params.x_size / point_row_counter / 4);
			road_nodes.Add(MakeShared<Node>(Node{FVector(x_val, y_val, 0)}));
		}
	}
	for (auto& r : road_nodes)
	{
		for (int iter = 0; iter < 10; iter++)
		{
			point_shift(r->get_point()->point, weighted_points);
		}
	}
	TArray<TTuple<double, TSharedPtr<Node>>> weighted_road_centers;
	for (auto& r : road_nodes)
	{
		if (FVector::Distance(r->get_FVector(),
		                      FVector{map_params.x_size / 2, map_params.y_size / 2, 0}) < (map_params.y_size /
			3))
		{
			bool is_break = false;
			for (auto riv : river)
			{
				if (FVector::Distance(r->get_FVector(), riv->get_FVector()) <
					map_params.river_road_distance)
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
					weight += (w.weight - FVector::Distance(
						r->get_FVector(), w.point));
				}
			}
			weighted_road_centers.Add(
				TTuple<double, TSharedPtr<Node>>{weight, r});
		}
	}

	Algo::Sort(weighted_road_centers,
	           [](const TTuple<double, TSharedPtr<Node>>& A,
	              const TTuple<double, TSharedPtr<Node>>& B)
	           {
		           return A.Get<0>() < B.Get<0>();
	           });
	// roads.Reset();
	// road.Add(weighted_road_centers[0].Value);
	for (int i = 1; i < weighted_road_centers.Num(); i++)
	{
		bool is_break = false;
		for (auto r : road_nodes)
		{
			if (FVector::Distance(weighted_road_centers[i].Value->get_FVector(),
			                      r->get_FVector()) < (map_params.y_size / 7))
			{
				is_break = true;
				break;
			}
		}
		if (!is_break)
		{
			road_nodes.Add(weighted_road_centers[i].Value);
		}
	}
	// placing bridges
	// int bridges_num = 15;
	// if (bridges_num > bridges.Num())
	// {
	// 	bridges_num = bridges.Num();
	// }
	// for (int i = 0; i < bridges_num; i++)
	// {
	// 	auto bridge1 = MakeShared<Node>(AllGeometry::create_segment_at_angle(
	// 		bridges[i].Key->get_FVector(), bridges[i].Value->get_FVector(), bridges[i].Value->get_FVector(), 0, 20));
	// 	auto bridge2 = MakeShared<Node>(AllGeometry::create_segment_at_angle(
	// 		bridges[i].Value->get_FVector(), bridges[i].Key->get_FVector(), bridges[i].Key->get_FVector(), 0, 20));
	// 	create_segment(roads, bridge1, bridge2, true, bridge1->get_node_type(), 5000);
	// 	roads.Add(bridge1);
	// 	roads.Add(bridge2);
	// }

	for (auto r : road_nodes)
	{
		weighted_points.Add(WeightedPoint{r->get_FVector(), -map_params.y_size / 7});
	}

	for (auto& r : road_nodes)
	{
		road_centers.Add(r);
	}

	for (auto& r : road_centers)
	{
		r->set_type(point_type::main_road);
	}
	if (map_params.city_plan == ECityPlan::combined)
	{
		for (int i = 0; i < road_centers.Num() - 1; i++)
		{
			auto local_road = road_centers;

			local_road.Sort(
				[i, this](const TSharedPtr<Node>& Item1,
				          const TSharedPtr<Node>& Item2)
				{
					return FVector::Distance(road_centers[i]->get_FVector(),
					                         Item1->get_FVector()) <
						FVector::Distance(road_centers[i]->get_FVector(),
						                  Item2->get_FVector());
				});
			int success_roads = 0;
			for (int j = 0; j < local_road.Num() - 1 && success_roads < 4; j++)
			{
				success_roads += create_guiding_road_segment(
					road_centers[i], local_road[j], true, point_type::main_road, river);
			}
		}
	}
	else if (map_params.city_plan == ECityPlan::rectangular)
	{
		map_params.max_road_length = map_params.av_road_length;
		map_params.min_road_length = map_params.av_road_length;
		map_params.road_forward_chance = 100;
		map_params.road_left_chance = 100;
		map_params.road_right_chance = 100;
		for (int i = 0; i < road_centers.Num() - 1; i++)
		{
			auto local_road = road_centers;

			local_road.Sort(
				[i, this](const TSharedPtr<Node>& Item1,
				          const TSharedPtr<Node>& Item2)
				{
					return FVector::Distance(road_centers[i]->get_FVector(),
					                         Item1->get_FVector()) <
						FVector::Distance(road_centers[i]->get_FVector(),
						                  Item2->get_FVector());
				});
			int success_roads = 0;
			for (int j = 0; j < local_road.Num() - 1 && success_roads < 4; j++)
			{
				success_roads += create_guiding_road_segment(
					road_centers[i], local_road[j], true,
					point_type::main_road, river);
			}
		}
	}
	else if (map_params.city_plan == ECityPlan::radial || map_params.city_plan ==
		ECityPlan::radial_circle)
	{
		TSharedPtr<Node> closest_center;
		float closest_dist = TNumericLimits<float>::Max();
		TArray<FVector> river_points;
		for (auto river_district : river_figures)
		{
			for (auto river_node : river_district->figure)
			{
				river_points.Add(river_node->get_FVector());
			}
		}
		for (auto r : road_centers)
		{
			if (FVector::Distance(r->get_FVector(), map_params.center) < closest_dist &&
				!is_point_in_river(r->get_FVector()))
			{
				if (!AllGeometry::is_point_near_figure(
					river_points, r->get_FVector(), 200))
				{
					closest_dist = FVector::Distance(r->get_FVector(), map_params.center);
					closest_center = r;
				}
			}
		}
		central_node = closest_center;
		// roads.Empty();
		road_nodes.AddUnique(closest_center);
		road_centers.Empty();
		road_centers.Add(closest_center);
		bridges.Empty();

		auto left_node = MakeShared<Node>(closest_center->get_FVector());
		left_node->set_FVector(FVector(closest_center->get_FVector().X, 0, 0));
		auto trade_path_angle = FMath::FRand() * 180;
		auto trade_path1 = AllGeometry::create_segment_at_angle(
			left_node->get_FVector(), closest_center->get_FVector(),
			closest_center->get_FVector(),
			trade_path_angle, 5000);
		auto trade_path2 = AllGeometry::create_segment_at_angle(
			left_node->get_FVector(), closest_center->get_FVector(),
			closest_center->get_FVector(),
			trade_path_angle + 180, 5000);
		FVector trade_path1_end;
		FVector trade_path2_end;
		auto trade_path1_end_point = AllGeometry::is_intersect_array(
			closest_center->get_FVector(), trade_path1, map_borders_array,
			false);
		auto trade_path2_end_point = AllGeometry::is_intersect_array(
			closest_center->get_FVector(), trade_path2, map_borders_array,
			false);
		if (trade_path1_end_point.IsSet() && trade_path2_end_point.IsSet())
		{
			trade_path1_end = trade_path1_end_point->Key;
			trade_path1_end = AllGeometry::create_segment_at_angle(
				closest_center->get_FVector(), trade_path1_end,
				closest_center->get_FVector(),
				0, FVector::Distance(closest_center->get_FVector(),
				                     trade_path1_end) - 0.01f);
			trade_path2_end = trade_path2_end_point->Key;
			trade_path2_end = AllGeometry::create_segment_at_angle(
				closest_center->get_FVector(), trade_path2_end,
				closest_center->get_FVector(),
				0, FVector::Distance(closest_center->get_FVector(),
				                     trade_path2_end) - 0.01f);
		}
		else return;

		auto trade_path1_end_node = MakeShared<Node>(trade_path1_end);
		auto trade_path2_end_node = MakeShared<Node>(trade_path2_end);
		trade_path1_end_node->unshrinkable = true;
		trade_path2_end_node->unshrinkable = true;
		create_guiding_road_segment(closest_center, trade_path1_end_node, true,
		                            point_type::main_road, river);
		create_guiding_road_segment(closest_center, trade_path2_end_node, true,
		                            point_type::main_road, river);

		TArray<TSharedPtr<Node>> circle_nodes;
		for (int angle = trade_path_angle + FMath::FRand() * 10 + 20; angle <
		     trade_path_angle + 180 - 20; angle += FMath::FRand() * 10 + 20)
		{
			double dist_from_center = map_params.av_distance * (FMath::FRand() * 0.5 +
				0.25);
			auto new_node_fvec = AllGeometry::create_segment_at_angle(
				left_node->get_FVector(), closest_center->get_FVector(),
				closest_center->get_FVector(),
				angle, dist_from_center);

			if (is_point_in_river(new_node_fvec))
			{
				continue;
			}
			TSharedPtr<Node> new_node(MakeShared<Node>(new_node_fvec));
			new_node->set_type(point_type::main_road);
			road_nodes.Add(new_node);
			if (create_guiding_road_segment(closest_center, new_node, true,
			                                point_type::main_road, river))
			{
				road_centers.Add(new_node);
			}
			if (map_params.city_plan == ECityPlan::radial_circle)
			{
				auto new_circle_node_fvec =
					AllGeometry::create_segment_at_angle(
						left_node->get_FVector(), closest_center->get_FVector(),
						closest_center->get_FVector(),
						angle, dist_from_center + map_params.av_road_length);
				soft_borders_array.Add(new_node->get_FVector());
				if (is_point_in_river(new_circle_node_fvec))
				{
					continue;
				}
				TSharedPtr<Node> new_wall_node(
					MakeShared<Node>(new_circle_node_fvec));
				new_wall_node->set_type(point_type::wall);
				road_nodes.Add(new_wall_node);
				circle_nodes.Add(new_wall_node);
			}
		}
		for (int angle = trade_path_angle + 180 + FMath::FRand() * 10 + 20;
		     angle < trade_path_angle + 360 - 20; angle += FMath::FRand() * 10 +
		     20)
		{
			double dist_from_center = map_params.av_distance * (FMath::FRand() * 0.5 +
				0.25);
			auto new_node_fvec = AllGeometry::create_segment_at_angle(
				left_node->get_FVector(), closest_center->get_FVector(),
				closest_center->get_FVector(),
				angle, dist_from_center);

			if (is_point_in_river(new_node_fvec))
			{
				continue;
			}
			TSharedPtr<Node> new_node(MakeShared<Node>(new_node_fvec));
			new_node->set_type(point_type::main_road);
			road_nodes.Add(new_node);
			if (create_guiding_road_segment(closest_center, new_node, true,
			                                point_type::main_road, river))
			{
				road_centers.Add(new_node);
			}
			if (map_params.city_plan == ECityPlan::radial_circle)
			{
				auto new_circle_node_fvec =
					AllGeometry::create_segment_at_angle(
						left_node->get_FVector(), closest_center->get_FVector(),
						closest_center->get_FVector(),
						angle, dist_from_center + map_params.av_road_length);
				soft_borders_array.Add(new_node->get_FVector());
				if (is_point_in_river(new_circle_node_fvec))
				{
					continue;
				}
				TSharedPtr<Node> new_wall_node(
					MakeShared<Node>(new_circle_node_fvec));
				new_wall_node->set_type(point_type::wall);
				road_nodes.Add(new_wall_node);
				circle_nodes.Add(new_wall_node);
			}
		}

		for (int i = 1; i <= circle_nodes.Num(); i++)
		{
			create_guiding_road_segment(circle_nodes[i - 1],
			                            circle_nodes[i % circle_nodes.Num()],
			                            true, point_type::wall, river);
		}
		TArray<TSharedPtr<Node>> local_road;
		// for (auto bridge_point : bridges)
		// {
		// 	local_road.AddUnique(bridge_point.Key);
		// 	local_road.AddUnique(bridge_point.Value);
		// }
		int success_roads = 0;
		for (int i = 0; i < local_road.Num() && success_roads <= 1; i++)
		{
			success_roads = 0;
			local_road.Sort(
				[i, this, local_road](const TSharedPtr<Node>& Item1,
				                      const TSharedPtr<Node>& Item2)
				{
					return FVector::Distance(local_road[i]->get_FVector(),
					                         Item1->get_FVector()) <
						FVector::Distance(local_road[i]->get_FVector(),
						                  Item2->get_FVector());
				});
			for (int j = 0; j < local_road.Num(); j++)
			{
				success_roads += create_guiding_road_segment(
					local_road[i], local_road[j], false, point_type::main_road, river);
				if (success_roads <= 1) break;
			}
		}
	}
}

bool TerrainGen::create_guiding_road_segment(
	const TSharedPtr<Node>& start_point, const TSharedPtr<Node>& end_point,
	bool is_through_river, point_type road_type, const TArray<TSharedPtr<Node>>& river)
{
	if (AllGeometry::is_intersect_array(start_point, end_point, river, true).
		IsSet() && !is_through_river)
	{
		return false;
	}
	if (FVector::Distance(start_point->get_FVector(), end_point->get_FVector())
		< 0.01)
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

		auto inter_point1 = AllGeometry::is_intersect_array(
			local_start_point, end_point, river, false);
		if (inter_point1.IsSet())
		{
			local_end_point = MakeShared<Node>(inter_point1->Key);
			local_end_point->set_type(road_type);
			FVector real_bridge_end;
			auto point1 = inter_point1->Value.Key;
			auto point2 = inter_point1->Value.Value;
			int angle = -90;
			// auto calc_angle = AllGeometry::calculate_angle_clock(inter_point1->Value.Key->get_FVector(),
			// 	inter_point1->Key, local_start_point->get_FVector());
			// if (calc_angle < 180)
			// {
			// 	angle = 90;
			// }
			if (!is_point_in_river(AllGeometry::create_segment_at_angle(
				point1->get_FVector(), point2->get_FVector(),
				inter_point1->Key, angle, 0.1)))
			{
				angle = 90;
			}
			road_nodes.AddUnique(local_end_point);
			TSharedPtr<Node> inter_point2 = MakeShared<Node>(
				local_end_point->get_FVector());
			for (int i = 10; ; i += 5)
			{
				auto point_end = AllGeometry::create_segment_at_angle(
					point1->get_FVector(), point2->get_FVector(),
					inter_point1->Key, angle, i);
				if (!is_point_in_river(point_end))
				{
					auto inter = AllGeometry::is_intersect_array(
						point_end, inter_point1->Key, river, false);
					if (inter.IsSet())
					{
						inter_point2->set_FVector(inter->Key);
					}
					else
					{
						return false;
					}
					// inter_point2->set_FVector(point_end);
					break;
				}
			}
			auto new_node = MakeShared<Node>(inter_point2->get_FVector());
			new_node->set_type(road_type);
			road_nodes.AddUnique(new_node);
			next_start_point = new_node;
			real_bridge_end = AllGeometry::create_segment_at_angle(
				local_end_point->get_FVector(), next_start_point->get_FVector(),
				next_start_point->get_FVector(), 0, 6);
			next_start_point->set_FVector(real_bridge_end);
			FVector real_bridge_start = AllGeometry::create_segment_at_angle(
				next_start_point->get_FVector(), local_end_point->get_FVector(),
				local_end_point->get_FVector(), 0, 6);
			local_end_point->set_FVector(real_bridge_start);
			next_start_point->set_FVector(real_bridge_end);
			create_segment(road_nodes, local_end_point, next_start_point, true,
			               road_type, 5000);
			// if (road_type != wall)
			// {
			// 	bridges.Add(TTuple<TSharedPtr<Node>, TSharedPtr<Node>>(local_end_point, next_start_point));
			// }
		}

		TSharedPtr<Node> old_node = local_start_point;
		int dist_times = FVector::Distance(local_start_point->get_FVector(),
		                                   local_end_point->get_FVector()) / (
			map_params.av_road_length);
		dist_times = dist_times < 1 ? 1 : dist_times;
		// UE_LOG(LogTemp, Warning, TEXT("количество раз %d"), dist_times);
		for (int i = 1; i <= dist_times; i++)
		{
			TSharedPtr<Node> node = MakeShared<Node>(0, 0, 0);
			node->set_type(road_type);
			node->set_FVector(local_start_point->get_FVector() +
				((local_end_point->get_FVector() - local_start_point->
					get_FVector()) / dist_times * i));
			if (i == dist_times)
			{
				node = local_end_point;
			}
			road_nodes.AddUnique(node);
			create_segment(road_nodes, old_node, node, true, road_type,
			               map_params.max_road_length);
			old_node = node;
		}
	}
	while (FVector::Distance(local_end_point->get_FVector(),
	                         end_point->get_FVector()) > 0.01 && tries <= 10);
	return true;
}

void TerrainGen::shrink_roads()
{
	int road_points = road_nodes.Num();
	int old_road_points = TNumericLimits<int>::Max();
	while (road_points != old_road_points)
	{
		old_road_points = road_nodes.Num();
		auto array = road_nodes;
		road_nodes.RemoveAll(
			[&](const TSharedPtr<Node>& node)
			{
				if (node->unshrinkable || node->unmovable)
				{
					return false;
				}
				TSharedPtr<Node> target_node = nullptr;
				for (auto conn : node->conn)
				{
					if (FVector::Dist(node->get_FVector(),
					                  conn->node->get_FVector()) <
						map_params.min_road_length)
					{
						target_node = conn->node;
						break;
					}
				}

				if (target_node.IsValid() && node.IsValid() && node->
					get_node_type() < target_node->get_node_type())
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
		road_points = road_nodes.Num();
	}
	road_points = road_nodes.Num();
	old_road_points = TNumericLimits<int>::Max();
	while (road_points != old_road_points)
	{
		old_road_points = road_nodes.Num();
		road_nodes.RemoveAll(
			[&](const TSharedPtr<Node>& node)
			{
				if (node->conn.Num() < 2)
				{
					node->delete_me();
					return true;
				}
				return false;
			});
		road_points = road_nodes.Num();
	}
}

void TerrainGen::create_usual_roads(const TArray<WeightedPoint>& weighted_points, const TArray<TSharedPtr<Node>>& river)
{
	road_nodes.Sort([this](auto Item1, auto Item2)
	{
		return FMath::FRand() < 0.5f;
	});
	TArray<TSharedPtr<Node>> add_road = road_nodes;

	for (auto road_node : road_nodes)
	{
		if (!road_node->is_used() && road_node->get_node_type() !=
			point_type::river)
		{
			bool is_end = false;
			if (road_node->conn.Num() == 1)
			{
				if (FMath::FRand() * 100 <= map_params.road_forward_chance)
				{
					auto length = FVector::Distance(
							road_node->get_FVector(),
							road_node->conn[0]->node->get_FVector()) +
						(FMath::FRand() * 40) - 20;
					if (length < map_params.min_new_road_length)
					{
						length = map_params.min_new_road_length;
					}
					if (length > map_params.max_road_length)
					{
						length = map_params.max_road_length;
					}
					double angle_in_degrees = (FMath::FRand() * 10) - 5;
					auto line1 = AllGeometry::create_segment_at_angle(
						road_node->conn[0]->node->get_FVector(),
						road_node->get_FVector(), road_node->get_FVector(),
						angle_in_degrees, length);

					TSharedPtr<Node> new_node = MakeShared<Node>(line1);
					new_node->set_type(point_type::road);
					bool is_possible = false;

					auto check_end = AllGeometry::create_segment_at_angle(
						road_node->conn[0]->node->get_FVector(),
						road_node->get_FVector(), road_node->get_FVector(),
						angle_in_degrees, 1);
					if (!is_point_in_river(check_end))
					{
						if (!soft_borders_array.IsEmpty())
						{
							if (AllGeometry::is_point_in_figure(
								line1, soft_borders_array))
							{
								is_possible = true;
							}
						}
						else
						{
							for (auto rc : road_centers)
							{
								if (FVector::Distance(rc->get_FVector(), line1)
									< rc->conn.Num() * (map_params.y_size / 28))
								{
									is_possible = true;
									break;
								}
							}
						}
					}

					if (is_possible)
					{
						// create_usual_road_segment(add_road, road_node, new_node);
						auto finish = create_segment(
							add_road, road_node, new_node, false,
							point_type::road, map_params.max_road_length);
						if (finish.IsSet() && (finish.GetValue() != new_node ||
							finish.GetValue()->get_node_type() ==
							point_type::river))
						{
							is_end = true;
						}
					}
				}
			}
			if ((road_node->conn.Num() == 2 || road_node->conn.Num() == 1) && !
				is_end)
			{
				if (FMath::FRand() * 100 <= map_params.road_left_chance)
				{
					auto length = FVector::Distance(
							road_node->get_FVector(),
							road_node->conn[0]->node->get_FVector()) +
						(FMath::FRand() * 40) - 20;
					if (length < map_params.min_new_road_length)
					{
						length = map_params.min_new_road_length;
					}
					if (length > map_params.max_road_length)
					{
						length = map_params.max_road_length;
					}
					double angle_in_degrees = 90 + (FMath::FRand() * 10) - 5;
					auto line2 = AllGeometry::create_segment_at_angle(
						road_node->conn[0]->node->get_FVector(),
						road_node->get_FVector(), road_node->get_FVector(),
						angle_in_degrees, length);
					TSharedPtr<Node> new_node2 = MakeShared<Node>(line2);
					new_node2->set_type(point_type::road);
					create_segment(add_road, road_node, new_node2, false,
					               point_type::road, map_params.max_road_length);
				}
				if (FMath::FRand() * 100 <= map_params.road_right_chance)
				{
					auto length = FVector::Distance(
							road_node->get_FVector(),
							road_node->conn[0]->node->get_FVector()) +
						(FMath::FRand() * 40) - 20;
					if (length < map_params.min_new_road_length)
					{
						length = map_params.min_new_road_length;
					}
					if (length > map_params.max_road_length)
					{
						length = map_params.max_road_length;
					}
					double angle_in_degrees = -90 + (FMath::FRand() * 10) - 5;
					auto line3 = AllGeometry::create_segment_at_angle(
						road_node->conn[0]->node->get_FVector(),
						road_node->get_FVector(), road_node->get_FVector(),
						angle_in_degrees, length);
					TSharedPtr<Node> new_node3 = MakeShared<Node>(line3);
					new_node3->set_type(point_type::road);
					create_segment(add_road, road_node, new_node3, false,
					               point_type::road, map_params.max_road_length);
				}
			}
		}

		road_node->set_used();
	}
	for (auto a : add_road)
	{
		bool is_in_road = false;
		for (auto r : road_nodes)
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
			road_nodes.AddUnique(a);
		}
	}

	for (int i = 0; i < road_nodes.Num(); i++)
	{
		if (!road_nodes[i]->is_used())
		{
			for (int count = 0; count < 3; count++)
			{
				move_road(road_nodes[i], weighted_points, river);
			}
		}
	}
}

TOptional<TSharedPtr<Node>> TerrainGen::create_segment(
	TArray<TSharedPtr<Node>>& array, TSharedPtr<Node> start_point,
	TSharedPtr<Node> end_point, bool to_exect_point, point_type type,
	double max_length)
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
		if ((end_point->get_FVector().X) > map_params.x_size)
		{
			end_point->set_FVector_X(map_params.x_size);
			force_end = true;
		}
		if ((end_point->get_FVector().Y) > map_params.y_size)
		{
			end_point->set_FVector_Y(map_params.y_size);
			force_end = true;
		}
		end_point->set_type(type);
		if (force_end)
		{
			return end_point;
		}
		int tries = 0;
		auto intersection = AllGeometry::is_intersect_array_clear(
			start_point, end_point, array, false);

		while (intersection.IsSet())
		{
			if (intersection.GetValue()->get_node_type() == point_type::wall &&
				type != point_type::wall)
			{
				return TSharedPtr<Node>();
			}
			end_point = intersection.GetValue();
			end_point->set_type(type);
			tries++;
			if (tries > 10 || FVector::Distance(start_point->get_FVector(),
			                                    end_point->get_FVector()) >
				max_length)
			{
				break;
			}
			intersection = AllGeometry::is_intersect_array_clear(
				start_point, end_point, array, false);
		}
		if (tries > 10 || FVector::Distance(start_point->get_FVector(),
		                                    end_point->get_FVector()) >
			max_length ||
			to_exect_point)
		{
			auto presise_intersect = AllGeometry::is_intersect_array(
				start_point->get_FVector(),
				backup_endpoint->get_FVector(), array, false);
			if (presise_intersect.IsSet())
			{
				end_point =
					insert_conn(presise_intersect->Value.Key,
					            presise_intersect->Value.Value,
					            presise_intersect->Key);
				end_point->set_type(type);
			}
		}
		bool is_river_intersect = false;
		if (type == point_type::road)
		{
			is_river_intersect = is_point_in_river(end_point->get_FVector());
		}
		if (!is_river_intersect)
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
	while (FVector::Distance(end_point->get_FVector(),
	                         backup_endpoint->get_FVector()) > 0.01);

	if (end_point->get_FVector() == backup_endpoint->get_FVector())
	{
		end_point = backup_endpoint;
	}
	return end_point;
}

void TerrainGen::point_shift(FVector& point, TArray<WeightedPoint> weighted_points)
{
	FVector bias(0, 0, 0);
	double biggest_weight = 0;
	for (int j = 0; j < weighted_points.Num(); j++)
	{
		double length = FVector::Distance(point, weighted_points[j].point);
		if (length < abs(weighted_points[j].weight) && biggest_weight <
			weighted_points[j].weight - length)
		{
			biggest_weight = weighted_points[j].weight - length;
			bias = ((point - weighted_points[j].point) / length) * (
				weighted_points[j].weight - length) / 50;
		}
	}
	point += bias;
}

void TerrainGen::get_closed_figures(TArray<TSharedPtr<Node>> nodes,
                                    TArray<TSharedPtr<District>>& fig_array,
                                    int figure_threshold)
{
	// bool is_river;
	// if (nodes.Num() > 0 && nodes[0]->get_node_type() == point_type::river)
	// {
	// 	is_river = true;
	// }
	for (auto node : nodes)
	{
		for (auto lconn : node->conn)
		{
			if (!lconn->figure->IsEmpty() || lconn->not_in_figure)
			{
				continue;
			}
			TSharedPtr<TArray<TSharedPtr<Node>>> figure_array = MakeShared<
				TArray<TSharedPtr<Node>>>();
			TArray<TSharedPtr<Conn>> conn_array;
			conn_array.Add(lconn);
			auto first_node = node;

			// first_node->print_connections();
			auto second_node = lconn->node;
			// second_node->print_connections();
			figure_array->Add(node);
			figure_array->Add(lconn->node);
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
						AllGeometry::calculate_angle(
							second_node->conn[i]->node->get_FVector(),
							second_node->get_FVector(),
							first_node->get_FVector(), true);

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
				figure_array->Add(rightest_node);
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
				if (
					(general_angle / (figure_array->Num() - 1) < 180
						/* && !is_river*/)
					/*|| (is_river && (general_angle / (figure_array->Num() - 1) > 180))*/)
				{
					// UE_LOG(LogTemp, Warning, TEXT("Фигура добавлена, размер %d"), figure_array->Num());
					fig_array.Add(MakeShared<District>(*figure_array));
				}
			}
		}
	}
}

void TerrainGen::get_river_figure(const TArray<TSharedPtr<Node>>& river)
{
	TArray<TSharedPtr<District>> river_fig_array;
	get_closed_figures(river, river_fig_array, 1000);
	if (river_fig_array.IsEmpty())
	{
		return;
	}
	// river_fig_array.Sort([this](const District& Item1, const District& Item2)
	// {
	// 	return Item1.figure.Num() > Item2.figure.Num();
	// });
	for (auto item : river_fig_array)
	{
		item->set_district_type(district_type::water);
		river_figures.Add(item);
	}
	// river_figures.RemoveAll(
	// 	[&](TSharedPtr<District>& A)
	// 	{
	// 		bool is_in = false;
	// 		for (auto point : custom_districts)
	// 		{
	// 			if (point.Value == district_type::water && A->
	// 				is_point_in_figure(point.Key))
	// 			{
	// 				is_in = true;
	// 				break;
	// 			}
	// 		}
	// 		return !is_in;
	// 	});
}

void TerrainGen::process_districts(TArray<TSharedPtr<District>>& districts)
{
	districts.Sort(
		[this](TSharedPtr<District> Item1, TSharedPtr<District> Item2)
		{
			return Item1->area > Item2->area;
		});
	double royal_area = 0;


	bool royal_found = false;
	for (auto& c : custom_districts)
	{
		// if (c.Value == district_type::tower)
		// {
		debug2_points_array.Add(c.Key);
		// }

		if (is_point_in_river(c.Key) && c.Value != district_type::water) continue;
		for (auto& d : districts)
		{
			if (d->get_district_type() != district_type::unknown) continue;
			if (d->is_point_in_figure(c.Key))
			{
				if (c.Value == district_type::royal)
				{
					royal_found = true;
				}
				d->set_district_type(c.Value);
				break;
			}
		}
	}

	// bool dock_found = false;
	for (auto& b : districts)
	{
		FVector center_of_dist;
		bool is_river = false;
		bool in_river = false;
		if (b->get_district_type() == district_type::unknown)
		{
			int fig_size = b->figure.Num();
			for (int i = 2; i < fig_size + 2; i++)
			{
				center_of_dist = (b->figure[i % fig_size]->get_FVector() +
					b->figure[(i - 1) % fig_size]->get_FVector() +
					b->figure[(i - 2) % fig_size]->get_FVector()) / 3;
				if (is_point_in_river(center_of_dist) && b->is_point_in_figure(
					center_of_dist))
				{
					in_river = true;
					break;
				}
			}
			if (in_river)
			{
				b->set_district_type(district_type::water);
				continue;
			}
		}
		if (b->get_district_type() == district_type::unknown && !royal_found &&
			royal_area == 0)
		{
			bool point1 = false;
			int is_in_main = 0;
			for (auto& p : b->figure)
			{
				if (is_river)
				{
					break;
				}
				if (FVector::Distance(p->get_FVector(), map_params.center) < (map_params.x_size +
					map_params.y_size) / 10)
				{
					point1 = true;
				}
				if (p->get_node_type() == point_type::main_road)
				{
					is_in_main++;
				}
				if (point1 && is_in_main >= 3)
				{
					b->set_district_type(district_type::royal);
					royal_area += b->area;
					break;
				}
			}
		}
		if (b->get_district_type() == district_type::unknown && royal_area > 0 && !
			royal_found)
		{
			for (auto p : b->figure)
			{
				for (auto district_near : p->get_point()->districts_nearby)
				{
					if (district_near == district_type::royal)
					{
						b->set_district_type(district_type::royal);
						royal_area += b->area;
						break;
					}
				}
				if (b->get_district_type() == district_type::royal)
				{
					break;
				}
			}
		}
		if (b->get_district_type() == district_type::royal && royal_area > 1000)
		{
			royal_found = true;
		}
		if (b->get_district_type() == district_type::unknown)
		{
			bool is_near_royal = false;
			bool is_near_dock = false;
			for (auto p : b->figure)
			{
				if (p->get_point()->districts_nearby.Contains(
					district_type::royal))
				{
					is_near_royal = true;
				}
				if (p->get_point()->districts_nearby.Contains(
					district_type::water))
				{
					is_near_dock = true;
				}
			}
			if (!is_near_royal && is_near_dock)
			{
				b->set_district_type(district_type::dock);
			}
			if (is_near_royal && !is_near_dock && b->area > 6000)
			{
				b->set_district_type(district_type::luxury);
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
			if (b->get_district_type() != district_type::unknown)
			{
				named_districts++;
			}
			else
			{
				TMap<district_type, int32> ElementCount;
				for (auto& fig : b->figure)
				{
					for (auto district : fig->get_point()->districts_nearby)
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
					case district_type::tower:
					case district_type::empty:
					case district_type::unknown:
					case district_type::water:
					default:
						break;
					}
				}
				if (districts_near == 0)
				{
					continue;
				}
				int koeff = (dock_count * (-4) + royal_count * 6 + luxury_count
					* 3 + slums_count * (-8) +
					residential_count * 2) / districts_near;

				if (koeff <= -7 && luxury_count == 0 && royal_count == 0)
				{
					b->set_district_type(district_type::slums);
				}
				else if (koeff >= 0 && koeff < 4)
				{
					b->set_district_type(district_type::residential);
				}
				else if (koeff >= 4 && slums_count != 0 && dock_count != 0)
				{
					b->set_district_type(district_type::luxury);
				}
				else
				{
					int rand_val;
					rand_val = FMath::FRand() * 100;
					if (rand_val > 85 && slums_count != 0 && dock_count != 0)
					{
						b->set_district_type(district_type::luxury);
					}
					else if (rand_val > 50)
					{
						b->set_district_type(district_type::residential);
					}
					else if (luxury_count == 0 && royal_count == 0)
					{
						b->set_district_type(district_type::slums);
					}
				}
			}
		}
	}
	while (named_districts < districts_count && old_named_districts != named_districts);

	districts.RemoveAll([this](TSharedPtr<District>& district)
	{
		district->self_figure = district->shrink_figure_with_roads(district->figure, map_params.road_width,
		                                                           map_params.main_road_width);
		return district->self_figure.IsEmpty();
	});
}

void TerrainGen::process_lights()
{
	TArray<FVector> road_lantern_anchors;
	for (auto& street : streets_array)
	{
		if (street->type == point_type::main_road)
		{
			TArray<TSharedPtr<Node>> street_vertices = street->get_street_vertices();
			for (int i = 1; i < street->street_vertices.Num(); i++)
			{
				FVector beg = street->street_vertices[i - 1]->get_FVector();
				FVector end = street->street_vertices[i]->get_FVector();
				float dist = FVector::Dist(beg, end);
				int count = dist / map_params.main_road_lights_dist;
				for (int c = 0; c < count; c++)
				{
					FVector cur_p = beg + (end - beg) / count * c;
					road_lantern_anchors.AddUnique(cur_p);
				}
			}
		}
	}

	for (auto& d : shapes_array)
	{
		if (d->get_district_type() == district_type::residential || d->get_district_type() == district_type::luxury ||
			d->get_district_type() == district_type::royal)
		{
			bool is_found = false;

			FVector closest_point(0, 0, 0);
			FVector closest_anchor(0, 0, 0);
			float closest_dist = 100000;
			for (auto& a : road_lantern_anchors)
			{
				bool is_near = false;
				for (auto& point : d->self_figure)
				{
					if (FVector::Distance(point.point, a) < map_params.max_road_length)
					{
						is_near = true;
						break;
					}
				}
				if (!is_near) continue;
				is_found = false;
				closest_dist = 100000;
				for (int i = 1; i < d->self_figure.Num(); i++)
				{
					FVector closest = AllGeometry::point_to_seg_distance_get_closest(
						d->self_figure[i].point, d->self_figure[i - 1].point, a);
					auto dist = FVector::Dist(closest, a);
					if (dist < closest_dist && dist < map_params.main_road_width)
					{
						closest_dist = dist;
						closest_point = closest;
						closest_anchor = a;
						is_found = true;
					}
				}
				if (is_found)
				{
					PointObject l(closest_point);
					FVector f2 = closest_point;
					FVector f3 = closest_point;
					f3.X += 1000;
					auto angle = AllGeometry::calculate_angle_clock(closest_anchor, f2, f3);
					angle = FMath::Fmod(angle + 270.0f, 360.0f);

					l.set_angle(angle);
					point_objects_array.Add(MakeShared<PointObject>(l));
				}
			}
		}
	}
	for (auto& l : point_objects_array)
	{
		debug_points_array.Add(l->position);
	}
}


void TerrainGen::process_houses(TSharedPtr<District> district)
{
	if (district->get_district_type() == district_type::water)
	{
		return;
	}
	FVector district_center(0, 0, 0);
	double left_point = TNumericLimits<double>::Max();
	double down_point = TNumericLimits<double>::Max();
	double right_point = 0;
	double up_point = 0;
	for (auto p : district->self_figure)
	{
		district_center += p.point;
		left_point = p.point.X < left_point ? p.point.X : left_point;
		right_point = p.point.X > right_point ? p.point.X : right_point;
		down_point = p.point.Y < down_point ? p.point.Y : down_point;
		up_point = p.point.Y > up_point ? p.point.Y : up_point;
	}
	district_center /= district->self_figure.Num();
	if (district->get_district_type() == district_type::luxury)
	{
		for (int i = 1; i <= 100; i++)
		{
			double angle = FMath::FRand() * 180;
			double point_x = FMath::FRand() * (right_point - left_point) + left_point;
			double point_y = FMath::FRand() * (up_point - down_point) + down_point;
			FVector center_point(point_x, point_y, 0);
			FVector left_center_point(0, point_y, 0);
			double width = FMath::FRandRange(lh_params.MinHouseX, lh_params.MaxHouseX);
			double length = FMath::FRandRange(lh_params.MinHouseY, lh_params.MaxHouseY);
			double height = FMath::FRandRange(lh_params.MinHouseZ, lh_params.MaxHouseZ);
			FVector point_beg = AllGeometry::create_segment_at_angle(left_center_point, center_point, center_point,
			                                                         angle, length / 2);
			FVector point_end = AllGeometry::create_segment_at_angle(left_center_point, center_point, center_point,
			                                                         angle + 180, length / 2);
			TArray<FVector> figure{point_beg, point_end};
			if (district->create_house(figure, width, height, "House"))
			{
				break;
			}
		}
	}
	else if (district->get_district_type() == district_type::residential)
	{
		int self_figure_count = district->self_figure.Num();
		int count_without_end = district->self_figure[0] == district->self_figure[self_figure_count - 1]
			                        ? self_figure_count - 1
			                        : self_figure_count;
		for (int i = 0; i < self_figure_count - 1; i++)
		{
			int i0 = (i - 2 + count_without_end) % count_without_end;
			int i1 = (i - 1 + count_without_end) % count_without_end;
			int i2 = i;
			int i3 = (i + 1) % count_without_end;

			FVector point0 = district->self_figure[i0].point;
			FVector point1 = district->self_figure[i1].point;
			FVector point2 = district->self_figure[i2].point;
			FVector point3 = district->self_figure[i3].point;

			auto angle1 = AllGeometry::calculate_angle_clock(point0, point1, point2);
			auto angle2 = AllGeometry::calculate_angle_clock(point1, point2, point3);
			// auto angle3 = AllGeometry::calculate_angle_counterclock(point0, point1, point2);
			// auto angle4 = AllGeometry::calculate_angle_counterclock(point1, point2, point3);


			FVector point1_inner_end = AllGeometry::create_segment_at_angle(point0, point1, point1, angle1 / 2,
			                                                                rh_params.InnerSidewalkWidth / FMath::Sin(
				                                                                FMath::DegreesToRadians(angle1 / 2)));
			FVector point2_inner_end = AllGeometry::create_segment_at_angle(point1, point2, point2, angle2 / 2,
			                                                                rh_params.InnerSidewalkWidth / FMath::Sin(
				                                                                FMath::DegreesToRadians(angle2 / 2)));
			FVector point1_outer_end = AllGeometry::create_segment_at_angle(point0, point1, point1, angle1 / 2 - 180,
			                                                                rh_params.OuterSidewalkWidth / FMath::Sin(
				                                                                FMath::DegreesToRadians(angle1 / 2)));
			FVector point2_outer_end = AllGeometry::create_segment_at_angle(point1, point2, point2, angle2 / 2 - 180,
			                                                                rh_params.OuterSidewalkWidth / FMath::Sin(
				                                                                FMath::DegreesToRadians(angle2 / 2)));

			TArray<FVector> figure{point2_inner_end, point1_inner_end, point1_outer_end, point2_outer_end};
			district->create_house(figure, 1, "Pavement", true);
		}
		for (int i = 1; i <= self_figure_count; i++)
		{
			FVector point1 = district->self_figure[i - 1].point;
			FVector point2 = district->self_figure[i % self_figure_count].point;

			double dist = FVector::Distance(point1, point2);
			if (dist >= map_params.x_size)
			{
				continue;
			}
			double general_width = 0;
			if (rh_params.MinHouseY * rh_params.MinHouseX > rh_params.MaxArea) continue;
			while (general_width + rh_params.MinHouseY < dist)
			{
				bool general_width_changed = false;
				for (int counter = 0; counter < 10; counter++)
				{
					if (FMath::FRand() * 100 <= rh_params.HouseChance)
					{
						int NumStepsX = (rh_params.MaxHouseX - rh_params.MinHouseX) / rh_params.SizeStep;
						int NumStepsY = (rh_params.MaxHouseY - rh_params.MinHouseY) / rh_params.SizeStep;
						double width = rh_params.MinHouseX + FMath::RandRange(0, NumStepsX) * rh_params.SizeStep;
						double length = rh_params.MinHouseY + FMath::RandRange(0, NumStepsY) * rh_params.SizeStep;
						if (width / length <= rh_params.MinHouseSidesRatio || width / length >= rh_params.
							MaxHouseSidesRatio || length * width >= rh_params.MaxArea)
						{
							continue;
						}
						double height = FMath::RandRange(rh_params.MinHouseZ, rh_params.MaxHouseZ);
						FVector point_beg = AllGeometry::create_segment_at_angle(point1, point2,
							(point2 - point1) / dist * (general_width + (width / 2)) + point1, 90,
							rh_params.InnerSidewalkWidth + 1);
						FVector point_end =
							AllGeometry::create_segment_at_angle(
								point1, point2, point_beg, 90, length);
						FVector point_beg2 = AllGeometry::create_segment_at_angle(point_beg, point_end,
							point_end, 90, width);
						FVector point_beg3 = AllGeometry::create_segment_at_angle(point_end, point_beg2,
							point_beg2, 90, length);

						TArray<FVector> figure{point_beg3, point_beg, point_end, point_beg2};
						if (!district->create_house(figure, height, "House", false))
						{
							continue;
						}
						general_width += width + FMath::RandRange(rh_params.MinSpaceBetweenHouses,
						                                          rh_params.MaxSpaceBetweenHouses);
						general_width_changed = true;
						break;
					}
					else
					{
						general_width += FMath::RandRange(rh_params.MinSpaceBetweenHouses,
						                                  rh_params.MaxSpaceBetweenHouses)
							+ FMath::RandRange(rh_params.MinSpaceBetweenHouses,
							                   rh_params.MaxSpaceBetweenHouses);
						general_width_changed = true;
					}
				}
				if (!general_width_changed)
				{
					general_width += FMath::RandRange(rh_params.MinHouseY, rh_params.MaxHouseY);
				}
			}
		}
	}
	else if (district->get_district_type() == district_type::slums)
	{
		for (int i = 1; i <= 100; i++)
		{
			double angle = FMath::FRand() * 180;
			double point_x = FMath::FRand() * (right_point - left_point) + left_point;
			double point_y = FMath::FRand() * (up_point - down_point) + down_point;
			FVector center_point(point_x, point_y, 0);
			FVector left_center_point(0, point_y, 0);
			double width = FMath::FRandRange(sh_params.MinHouseX, sh_params.MaxHouseX);
			double length = FMath::FRandRange(sh_params.MinHouseY, sh_params.MaxHouseY);
			double height = FMath::FRandRange(sh_params.MinHouseZ, sh_params.MaxHouseZ);
			FVector point_beg = AllGeometry::create_segment_at_angle(left_center_point, center_point, center_point,
			                                                         angle,
			                                                         length / 2);
			FVector point_end = AllGeometry::create_segment_at_angle(left_center_point, center_point, center_point,
			                                                         angle + 180,
			                                                         length / 2);
			TArray<FVector> figure{point_beg, point_end};
			district->create_house(figure, width, height, "House");
		}
	}
}

void TerrainGen::create_special_district(TArray<FVector>& figure,
                                         point_type type, FVector point)
{
	TArray<TSharedPtr<Node>> nodes;
	TArray<TSharedPtr<Point>> points;
	for (auto f : figure)
	{
		auto new_node = MakeShared<Node>(f);
		new_node->set_type(type);
		road_nodes.AddUnique(new_node);
		nodes.AddUnique(new_node);
		points.AddUnique(new_node->get_point());
	}
	create_special_district_by_nodes(nodes, type, point);
}

void TerrainGen::create_special_district_by_nodes(TArray<TSharedPtr<Node>>& nodes, point_type type, FVector point)
{
	for (int i = 1; i <= nodes.Num(); i++)
	{
		create_segment(road_nodes, nodes[i - 1], nodes[i % nodes.Num()], true, type, 5000);
	}
	create_segment(road_nodes, nodes.Last(), nodes[0], true, type, 5000);

	TArray<FVector> inner_figure;
	for (auto f : nodes)
	{
		auto new_inner_point = f->get_FVector();
		inner_figure.Add(new_inner_point);
	}
	if (*nodes.begin() != *nodes.end())
	{
		inner_figure.Add(nodes[0]->get_FVector());
	}
	// that's fucked up. 
	FVector Center = FVector::ZeroVector;
	for (const FVector& Point : inner_figure)
	{
		Center += Point;
	}
	Center /= inner_figure.Num();
	for (FVector& Point : inner_figure)
	{
		Point = Center + (Point - Center) * 0.9999;
	}

	road_nodes.RemoveAll(
		[&, point](const TSharedPtr<Node>& node)
		{
			if (AllGeometry::is_point_in_figure(node->get_FVector(), inner_figure)
			)
			{
				// debug_points_array.Add(node->get_FVector());
				node->delete_me();
				return true;
			}
			return false;
		});
}

void TerrainGen::create_circle(FVector point, double radius, district_type type,
                               point_type road_type, int vertex_count)
{
	custom_districts.Add(TTuple<FVector, district_type>(point, type));
	FVector left_point = point;
	left_point.X = 0;
	TArray<FVector> figure;
	for (int i = 1; i <= vertex_count; i++)
	{
		figure.Add(AllGeometry::create_segment_at_angle(
			left_point, point, point, 360 / vertex_count * i, radius));
	}
	create_special_district(figure, road_type, point);
}

void TerrainGen::create_circle_by_existing_nodes(FVector central_point, double radius, double interval,
                                                 district_type type, point_type road_type, int vertex_count,
                                                 bool sticky_river, bool sticky_walls)
{
	custom_districts.Add(TTuple<FVector, district_type>(central_point, type));

	TArray<TSharedPtr<Node>> road_nodes_near{};
	for (auto& rn : road_nodes)
	{
		auto dist = FVector::Distance(central_point, rn->get_FVector());
		if (dist > radius - interval && dist < radius + interval)
		{
			road_nodes_near.AddUnique(rn);
		}
	}
	TArray<FVector> figure1;
	TArray<FVector> figure2;
	TArray<TSharedPtr<Node>> resulting_figure;

	FVector left_point = central_point;
	left_point.X = 0;
	FVector p1_beg = AllGeometry::create_segment_at_angle(
		left_point, central_point, central_point, 360 / vertex_count, radius - interval);
	FVector p2_beg = AllGeometry::create_segment_at_angle(
		left_point, central_point, central_point, 360 / vertex_count, radius + interval);

	figure1.Add(p1_beg);
	figure1.Add(p2_beg);
	// debug_points_array.Add(p1_beg);
	// debug_points_array.Add(p2_beg);
	for (int i = 2; i <= vertex_count; i++)
	{
		FVector p1 = AllGeometry::create_segment_at_angle(
			left_point, central_point, central_point, 360 / vertex_count * i, radius + interval);
		FVector p2 = AllGeometry::create_segment_at_angle(
			left_point, central_point, central_point, 360 / vertex_count * i, radius - interval);
		figure1.Add(p1);
		figure1.Add(p2);
		// debug_points_array.Add(p1);
		// debug_points_array.Add(p2);
		figure2.Add(p2);
		figure2.Add(p1);
		float distance = 0;
		TOptional<TSharedPtr<Node>> cur_node;
		for (auto& rnr : road_nodes_near)
		{
			if (FVector::Distance(rnr->get_FVector(), central_point) > distance && AllGeometry::is_point_in_figure(
				rnr->get_FVector(), figure1))
			{
				cur_node = rnr;
				distance = FVector::Distance(rnr->get_FVector(), central_point);
				if ((sticky_river && rnr->get_node_type() == point_type::river) || (sticky_walls && rnr->get_node_type()
					== point_type::wall))
					break;
			}
		}
		if (cur_node.IsSet())
		{
			resulting_figure.Add(cur_node.GetValue());
		}

		figure1 = figure2;
		figure2.Empty();
	}

	if (resulting_figure.Num() > 2)
	{
		create_special_district_by_nodes(resulting_figure, road_type, central_point);
	}
}

void TerrainGen::process_streets(TArray<TSharedPtr<Node>> nodes,
                                 TArray<TSharedPtr<Street>>& fig_array,
                                 point_type type, bool is_persistent)
{
	point_type type_used;
	type_used = type != point_type::unidentified ? type : point_type::road;
	for (auto node : nodes)
	{
		for (auto nconn : node->conn)
		{
			if (nconn->in_street)
			{
				continue;
			}
			if (type != point_type::unidentified && (node->get_node_type() !=
				type_used || nconn->node->get_node_type() != type_used))
			{
				continue;
			}
			if (node->get_node_type() != nconn->node->get_node_type() &&
				is_persistent)
			{
				continue;
			}

			auto street_points = MakeShared<TArray<TSharedPtr<Node>>>();

			street_points->Add(node);
			street_points->Add(nconn->node);

			auto first_point = node;
			auto second_point = nconn->node;
			TArray<TSharedPtr<Conn>> conn_array{};
			auto next_point = first_point->get_next_point(
				second_point->get_point());
			auto prev_point = first_point->get_prev_point(
				second_point->get_point());
			if (next_point.IsSet() && prev_point.IsSet())
			{
				if (next_point.GetValue()->in_street || prev_point.GetValue()->
				                                                   in_street)
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
			first_point = node;
			second_point = nconn->node;
			int counter = 0;
			do
			{
				counter++;
				is_next_found = false;
				third_point = get_next_road_node(
					first_point, second_point, type_used, is_persistent);
				if (third_point)
				{
					auto second_next = second_point->get_next_point(
						third_point->get_point());
					auto second_prev = second_point->get_prev_point(
						third_point->get_point());

					if (second_next.IsSet() && second_prev.IsSet())
					{
						conn_array.Add(second_next.GetValue());
						conn_array.Add(second_prev.GetValue());
						street_points->Add(third_point);
						first_point = second_point;
						second_point = third_point;
						is_next_found = true;
					}
				}
			}
			while (is_next_found && counter < 13);
			first_point = nconn->node;
			second_point = node;
			counter = 0;
			do
			{
				counter++;
				is_next_found = false;
				third_point = get_next_road_node(
					first_point, second_point, type_used, is_persistent);
				if (third_point)
				{
					auto second_next = second_point->get_next_point(
						third_point->get_point());
					auto second_prev = second_point->get_prev_point(
						third_point->get_point());
					if (second_next.IsSet() && second_prev.IsSet())
					{
						conn_array.Add(second_next.GetValue());
						conn_array.Add(second_prev.GetValue());
						street_points->Insert(third_point, 0);
						first_point = second_point;
						second_point = third_point;
						is_next_found = true;
					}
				}
			}
			while (is_next_found && counter < 13);
			if (street_points->Num() == 2 && is_persistent)
			{
				continue;
			}
			Street street(*street_points);
			street.type = type_used;
			auto street_ptr = MakeShared<Street>(street);
			fig_array.Add(street_ptr);
			for (auto conn : conn_array)
			{
				conn->in_street = true;
				conn->set_street(street_ptr);
			}
		}
	}
}

TArray<TSharedPtr<Street>> TerrainGen::process_segments(
	TArray<TSharedPtr<Street>>& fig_array)
{
	TArray<TSharedPtr<Street>> segments{};
	for (auto& street : fig_array)
	{
		if (street->street_vertices.Num() < 2) break;
		TSharedPtr<Street> cur_street(
			MakeShared<Street>(TArray<TSharedPtr<Node>>()));
		cur_street->street_vertices.Add(street->street_vertices[0]);
		for (int i = 1; i < street->street_vertices.Num(); i++)
		{
			if (street->street_vertices[i]->conn.Num() > 2 || i == street->
			                                                       street_vertices.Num() - 1)
			{
				cur_street->street_vertices.Add(street->street_vertices[i]);
				cur_street->type = street->type;
				segments.Add(cur_street);
				cur_street = TSharedPtr<Street>(
					MakeShared<Street>(TArray<TSharedPtr<Node>>()));
			}
			auto next = street->street_vertices[i - 1]->get_next_point(
				street->street_vertices[i]->get_point());
			auto prev = street->street_vertices[i - 1]->get_prev_point(
				street->street_vertices[i]->get_point());
			if (next.IsSet())
			{
				next.GetValue()->set_segment(cur_street);
			}
			if (prev.IsSet())
			{
				prev.GetValue()->set_segment(cur_street);
			}
			cur_street->street_vertices.Add(street->street_vertices[i]);
		}
		cur_street->type = street->type;
		segments.Add(cur_street);
	}
	return segments;
}

TSharedPtr<Node> TerrainGen::get_next_road_node(TSharedPtr<Node> first_point,
                                                TSharedPtr<Node> second_point,
                                                point_type type,
                                                bool is_persistent)
{
	auto third_point = second_point;
	TSortedMap<double, TSharedPtr<Node>> angles;
	for (auto second_node_conn : second_point->conn)
	{
		third_point = second_node_conn->node;

		if ((is_persistent && third_point->get_node_type() != type) ||
			third_point == first_point)
		{
			continue;
		}
		auto angle_before_abs = AllGeometry::calculate_angle(
			first_point->get_FVector(), second_point->get_FVector(),
			third_point->get_FVector(), false);
		auto angle = FMath::Abs(180 - angle_before_abs);
		if (angle < 25 || (angle_before_abs > 90 && second_point->conn.Num() ==
			2))
		{
			angles.Add(angle, third_point);
		}
	}

	for (auto current_val : angles)
	{
		auto supposed_third_point = current_val.Value;
		auto second_next = second_point->get_next_point(
			supposed_third_point->get_point());
		auto second_prev = second_point->get_prev_point(
			supposed_third_point->get_point());
		if (second_next.IsSet() && second_prev.IsSet())
		{
			if (second_next.GetValue()->in_street || second_prev.GetValue()->
			                                                     in_street)
			{
				continue;
			}
			return supposed_third_point;
			// street_points->Insert(supposed_third_point, 0);
		}
		break;
	}
	return TSharedPtr<Node>();
}
