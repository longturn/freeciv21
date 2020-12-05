/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/
#ifndef FC__COMMENTS_H
#define FC__COMMENTS_H



#define COMMENTS_FILE_NAME "comments-3.1.txt"

struct section_file;

bool comments_load(void);
void comments_free(void);

void comment_file_header(struct section_file *sfile);

void comment_buildings(struct section_file *sfile);
void comment_tech_classes(struct section_file *sfile);
void comment_techs(struct section_file *sfile);
void comment_govs(struct section_file *sfile);
void comment_policies(struct section_file *sfile);
void comment_uclasses(struct section_file *sfile);
void comment_utypes(struct section_file *sfile);
void comment_terrains(struct section_file *sfile);
void comment_resources(struct section_file *sfile);
void comment_extras(struct section_file *sfile);
void comment_bases(struct section_file *sfile);
void comment_roads(struct section_file *sfile);
void comment_styles(struct section_file *sfile);
void comment_citystyles(struct section_file *sfile);
void comment_musicstyles(struct section_file *sfile);
void comment_effects(struct section_file *sfile);
void comment_disasters(struct section_file *sfile);
void comment_achievements(struct section_file *sfile);
void comment_goods(struct section_file *sfile);
void comment_enablers(struct section_file *sfile);
void comment_specialists(struct section_file *sfile);
void comment_nationsets(struct section_file *sfile);
void comment_nationgroups(struct section_file *sfile);
void comment_nations(struct section_file *sfile);
void comment_clauses(struct section_file *sfile);



#endif /* FC__COMMENTS_H */
