#pragma once

#include <MapWizard/AllGeometry.h>
#include <MapWizard/MainTerrain.h>

struct CustomDistrNodes
{
	CustomDistrNodes(TSharedPtr<Node> node1_, TSharedPtr<Node> node2_,
	                 TSharedPtr<Node> node3_): node1(node1_)
	                                           , node2(node2_)
	                                           , node3(node3_)
	{
	};
	TSharedPtr<Node> node1;
	TSharedPtr<Node> node2;
	TSharedPtr<Node> node3;
};

class TerrainGen
{
public:
	FVector center;
	double av_distance;
	double av_river_length;
	double max_river_length;
	double min_new_road_length;
	double min_road_length;
	double av_road_length;
	double max_road_length;
	double river_road_distance;
	double x_size;
	double y_size;
	double road_left_chance;
	double road_forward_chance;
	double road_right_chance;
	double main_road_width;
	double road_width;
	ECityPlan city_plan;
	EDrawStage draw_stage;
	EWaterType water_type;

	bool did_river_multiplied = false;

	TArray<TSharedPtr<District>> river_figures;

	TerrainGen(FMapParams& map_params) : center(map_params.center)
	                                     , av_distance(map_params.av_distance)
	                                     , av_river_length(
		                                     map_params.av_river_length)
	                                     , max_river_length(
		                                     map_params.max_river_length)
	                                     , min_new_road_length(
		                                     map_params.min_new_road_length)
	                                     , min_road_length(
		                                     map_params.min_road_length)
	                                     , av_road_length(
		                                     map_params.av_road_length)
	                                     , max_road_length(
		                                     map_params.max_road_length)
	                                     , river_road_distance(
		                                     map_params.river_road_distance)
	                                     , x_size(map_params.x_size)
	                                     , y_size(map_params.y_size)
	                                     , road_left_chance(
		                                     map_params.road_left_chance)
	                                     , road_forward_chance(
		                                     map_params.road_forward_chance)
	                                     , road_right_chance(
		                                     map_params.road_right_chance)
	                                     , main_road_width(
		                                     map_params.main_road_width)
	                                     , road_width(map_params.road_width)
	                                     , city_plan(map_params.city_plan)
	                                     , draw_stage(map_params.draw_stage)
	                                     , water_type(map_params.water_type)
	{
	}

	void create_terrain(TArray<TSharedPtr<Node>>& roads_,
	                    TArray<TSharedPtr<District>>& figures_array_,
	                    TArray<TSharedPtr<Street>>& streets_array_,
	                    TArray<TSharedPtr<Street>>& segments_array_,
	                    TArray<TSharedPtr<District>>& river_figure_,
	                    TArray<TSharedPtr<Node>>& map_borders_array_,
	                    TArray<FVector>& debug_points_array_);
	static void add_conn(const TSharedPtr<Node>& node1,
	                     const TSharedPtr<Node>& node2);
	static TSharedPtr<Node> insert_conn(const TSharedPtr<Node>& node1_to_insert,
	                                    const TSharedPtr<Node>& node2_to_insert,
	                                    FVector node3_point);
	void move_river(const TSharedPtr<Node>& node1,
	                const TSharedPtr<Node>& node2);
	void move_road(const TSharedPtr<Node>& node);
	void create_guiding_rivers();
	void create_guiding_river_segment(const TSharedPtr<Node>& start_point,
	                                  const TSharedPtr<Node>& end_point,
	                                  const TSharedPtr<Node>& start_point_left,
	                                  const TSharedPtr<Node>&
	                                  start_point_right);
	bool is_point_in_river(FVector point);
	void create_guiding_roads();
	void create_usual_roads();
	TOptional<TSharedPtr<Node>> create_segment(TArray<TSharedPtr<Node>>& array,
	                                           TSharedPtr<Node> start_point,
	                                           TSharedPtr<Node> end_point,
	                                           bool to_exect_point,
	                                           point_type type,
	                                           double max_length);
	bool create_guiding_road_segment(const TSharedPtr<Node>& start_point,
	                                 const TSharedPtr<Node>& end_point,
	                                 bool is_through_river,
	                                 point_type road_type);
	void shrink_roads();
	void point_shift(FVector& point);
	void get_closed_figures(TArray<TSharedPtr<Node>> nodes,
	                        TArray<TSharedPtr<District>>& fig_array,
	                        int figure_threshold);
	void get_river_figure();
	void process_districts(TArray<TSharedPtr<District>>& districts);
	static void process_houses(TSharedPtr<District> block);
	void create_special_district(TArray<FVector>& figure, point_type type,
	                             FVector point);
	void create_circle(FVector point, double radius, district_type type,
	                   point_type road_type, int vertex_count);
	void process_streets(TArray<TSharedPtr<Node>> nodes,
	                     TArray<TSharedPtr<Street>>& fig_array, point_type type,
	                     bool is_persistent);
	TArray<TSharedPtr<Street>> process_segments(
		TArray<TSharedPtr<Street>>& fig_array);
	TSharedPtr<Node> get_next_road_node(TSharedPtr<Node> first_point,
	                                    TSharedPtr<Node> second_point,
	                                    point_type type, bool is_persistent);

	void empty_all()
	{
		for (auto& node : river)
		{
			node.Reset();
		}
		river.Reset();
		for (auto& node : guiding_river)
		{
			node.Reset();
		}
		river.Reset();
		for (auto& node : bridges)
		{
			node.Key.Reset();
			node.Value.Reset();
		}
		bridges.Reset();
		for (auto& node : guididng_roads_array)
		{
			node.Reset();
		}
		river.Reset();
		for (auto& node : road_centers)
		{
			node.Reset();
		}
		road_centers.Reset();
		for (auto& node : map_borders_array)
		{
			node.Reset();
		}
		map_borders_array.Reset();
		for (auto& node : roads)
		{
			node.Reset();
		}
		roads.Reset();
	}

	TArray<TSharedPtr<District>> shapes_array;
	TArray<TSharedPtr<Node>> river{};
	TArray<TSharedPtr<Node>> guiding_river{};
	TArray<TTuple<TSharedPtr<Node>, TSharedPtr<Node>>> bridges{};
	TArray<TSharedPtr<Node>> road_centers{};
	TArray<FVector> map_points_array{};
	TArray<TSharedPtr<Node>> map_borders_array{};
	TArray<TSharedPtr<Street>> streets_array{};
	TArray<TSharedPtr<Street>> segments_array{};
	TArray<TSharedPtr<Node>> guididng_roads_array{};
	TArray<WeightedPoint> weighted_points{};
	TArray<TSharedPtr<Node>> roads{};
	TArray<FVector> soft_borders_array{};
	TSharedPtr<Node> central_node;
	TArray<TTuple<FVector, district_type>> custom_districts;
	TArray<CustomDistrNodes> custom_distr_nodes;
};
