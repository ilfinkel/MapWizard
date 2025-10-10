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
	FMapParams map_params;
	FResidentialHousesParams rh_params;
	FLuxuryHousesParams lh_params;
	FSlumsHousesParams sh_params;
	bool did_river_multiplied = false;

	TArray<TSharedPtr<District>> river_figures;

	TerrainGen(FMapParams& map_params_,
	           FResidentialHousesParams& residential_houses_params_, FLuxuryHousesParams& luxury_houses_params_, 
	           FSlumsHousesParams& slums_houses_params_) :
		map_params(map_params_),
	rh_params(residential_houses_params_),
	lh_params(luxury_houses_params_),
	sh_params(slums_houses_params_)
	{
	}

	void create_terrain(TArray<TSharedPtr<Node>>& roads_, TArray<TSharedPtr<District>>& figures_array_,
	                    TArray<TSharedPtr<Street>>& streets_array_, TArray<TSharedPtr<Street>>& segments_array_,
	                    TArray<TSharedPtr<District>>& river_figure_, TArray<TSharedPtr<Node>>& map_borders_array_,
	                    TArray<TSharedPtr<PointObject>>& point_objects_array, TArray<FVector>& debug_points_array_, TArray<FVector>& debug2_points_array_);
	void create_weighted_points(TArray<WeightedPoint>& weighted_points);
	void create_rivers(const TArray<WeightedPoint>& weighted_points, TArray<TSharedPtr<Node>>& river);
	static void add_conn(const TSharedPtr<Node>& node1, const TSharedPtr<Node>& node2);
	static TSharedPtr<Node> insert_conn(const TSharedPtr<Node>& node1_to_insert,
	                                    const TSharedPtr<Node>& node2_to_insert, FVector node3_point);
	void move_river(const TSharedPtr<Node>& node1, const TSharedPtr<Node>& node2,
	                const TArray<WeightedPoint>& weighted_points, TArray<TSharedPtr<Node>>& river);
	void move_road(const TSharedPtr<Node>& node, const TArray<WeightedPoint>& weighted_points,
	               const TArray<TSharedPtr<Node>>& river);
	void create_guiding_rivers(TArray<TSharedPtr<Node>>& river);
	void create_guiding_river_segment(const TSharedPtr<Node>& start_point, const TSharedPtr<Node>& end_point,
	                                  const TSharedPtr<Node>& start_point_left,
	                                  const TSharedPtr<Node>& start_point_right, TArray<TSharedPtr<Node>>& river);
	bool is_point_in_river(FVector point);
	void create_guiding_roads(TArray<WeightedPoint>& weighted_points, const TArray<TSharedPtr<Node>>& river);
	void create_usual_roads(const TArray<WeightedPoint>& weighted_points, const TArray<TSharedPtr<Node>>& river);
	TOptional<TSharedPtr<Node>> create_segment(TArray<TSharedPtr<Node>>& array, TSharedPtr<Node> start_point,
	                                           TSharedPtr<Node> end_point, bool to_exect_point, EPointType type,
	                                           double max_length);
	bool create_guiding_road_segment(const TSharedPtr<Node>& start_point, const TSharedPtr<Node>& end_point,
	                                 bool is_through_river, EPointType road_type,
	                                 const TArray<TSharedPtr<Node>>& river);
	void shrink_roads();
	void point_shift(FVector& point, TArray<WeightedPoint> weighted_points_);
	void get_closed_figures(TArray<TSharedPtr<Node>> nodes, TArray<TSharedPtr<District>>& fig_array,
	                        int figure_threshold);
	void get_river_figure(const TArray<TSharedPtr<Node>>& river);
	void process_districts(TArray<TSharedPtr<District>>& districts);
	void process_lights();
	void process_houses(TSharedPtr<District> block);
	void create_special_district(TArray<FVector>& figure, EPointType type, FVector point);
	void create_special_district_by_nodes(TArray<TSharedPtr<Node>>& figure, EPointType type, FVector point);
	void create_circle(FVector point, double radius, Edistrict_type type,
	                   EPointType road_type, int vertex_count);
	void create_circle_by_existing_nodes(FVector central_point, double radius, double interval, Edistrict_type type,
	                                     EPointType road_type, int vertex_count, bool sticky_river, bool sticky_walls);
	void process_streets(TArray<TSharedPtr<Node>> nodes, TArray<TSharedPtr<Street>>& fig_array, EPointType type,
	                     bool is_persistent);
	TArray<TSharedPtr<Street>> process_segments(
		TArray<TSharedPtr<Street>>& fig_array);
	TSharedPtr<Node> get_next_road_node(TSharedPtr<Node> first_point,
	                                    TSharedPtr<Node> second_point,
	                                    EPointType type, bool is_persistent);

	void empty_all(TArray<TSharedPtr<Node>>& river)
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
		for (auto& node : road_nodes)
		{
			node.Reset();
		}
		road_nodes.Reset();
	}

	TArray<TSharedPtr<District>> shapes_array;
	// TArray<TSharedPtr<Node>> river{};
	TArray<TSharedPtr<Node>> guiding_river{};
	TArray<TTuple<TSharedPtr<Node>, TSharedPtr<Node>>> bridges{};
	TArray<TSharedPtr<Node>> road_centers{};
	TArray<FVector> map_points_array{};
	TArray<TSharedPtr<Node>> map_borders_array{};
	TArray<TSharedPtr<Street>> streets_array{};
	TArray<TSharedPtr<Street>> segments_array{};
	TArray<TSharedPtr<Node>> guididng_roads_array{};
	// TArray<WeightedPoint> weighted_points{};
	TArray<TSharedPtr<Node>> road_nodes{};
	TArray<FVector> soft_borders_array{};
	TSharedPtr<Node> central_node;
	TArray<TTuple<FVector, Edistrict_type>> custom_districts;
	TArray<CustomDistrNodes> custom_distr_nodes;
	TArray<FVector> debug_points_array;
	TArray<FVector> debug2_points_array;
	TArray<TSharedPtr<PointObject>> point_objects_array;
};
