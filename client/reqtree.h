/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#pragma once

#include "canvas_g.h"

/* Requirements Tree
 *
 * This file provides functions for drawing a tree-like graph of
 * requirements.  This can be used for creating an interactive diagram
 * showing the dependencies of various sources.
 *
 * A tree must first be created with create_reqtree; this will do all of the
 * calculations needed for the tree and is a fairly expensive operation.
 * After creating the tree, the other functions may be used to access or
 * draw it.
 *
 * Currently only techs are supported (as sources and requirements).
 */

/****************************************************************************
  This structure desribes a node in a technology tree diagram.
  A node can by dummy or real. Real node describes a technology.
****************************************************************************/
struct tree_node {
  bool is_dummy;
  Tech_type_id tech;

  /* Incoming edges */
  int nrequire;
  struct tree_node **require;

  /* Outgoing edges */
  int nprovide;
  struct tree_node **provide;

  /* logical position on the diagram */
  int order, layer;

  /* Coordinates of the rectangle on the diagram in pixels */
  int node_x, node_y, node_width, node_height;

  /* for general purpose */
  int number;
};

/****************************************************************************
  Structure which describes abstract technology diagram.
  Nodes are ordered inside layers[] table.
****************************************************************************/
struct reqtree {
  int num_nodes;
  struct tree_node **nodes;

  int num_layers;
  /* size of each layer */
  int *layer_size;
  struct tree_node ***layers;

  /* in pixels */
  int diagram_width, diagram_height;
};

struct reqtree *create_reqtree(struct player *pplayer, bool show_all);
void destroy_reqtree(struct reqtree *tree);

void get_reqtree_dimensions(struct reqtree *tree, int *width, int *height);

void draw_reqtree(struct reqtree *tree, struct canvas *pcanvas, int canvas_x,
                  int canvas_y, int tt_x, int tt_y, int w, int h);

Tech_type_id get_tech_on_reqtree(struct reqtree *tree, int x, int y);
bool find_tech_on_reqtree(struct reqtree *tree, Tech_type_id tech, int *x,
                          int *y, int *w, int *h);


