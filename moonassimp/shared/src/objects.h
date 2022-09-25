/* The MIT License (MIT)
 *
 * Copyright (c) 2016 Stefano Trettel
 *
 * Software repository: MoonAssimp, https://github.com/stetre/moonassimp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef objectsDEFINED
#define objectsDEFINED

#include "udata.h"

/* Objects' metatabe names */
#define SCENE_MT "moonassimp_scene"
#define NODE_MT "moonassimp_node"
#define MESH_MT "moonassimp_mesh"
#define ANIMMESH_MT "moonassimp_animmesh"
#define MATERIAL_MT "moonassimp_material"
#define ANIMATION_MT "moonassimp_animation"
#define NODEANIM_MT "moonassimp_nodeanim"
#define MESHANIM_MT "moonassimp_meshanim"
#define TEXTURE_MT "moonassimp_texture"
#define LIGHT_MT "moonassimp_light"
#define CAMERA_MT "moonassimp_camera"
#define FACE_MT "moonassimp_face"
#define BONE_MT "moonassimp_bone"

/* internal type redefinitions */
#define scene_t struct aiScene
#define node_t struct aiNode
#define mesh_t struct aiMesh
#define animmesh_t struct aiAnimMesh
#define material_t struct aiMaterial
#define animation_t struct aiAnimation
#define nodeanim_t struct aiNodeAnim
#define meshanim_t struct aiMeshAnim
#define texture_t struct aiTexture
#define light_t struct aiLight
#define camera_t struct aiCamera
#define face_t struct aiFace
#define bone_t struct aiBone
#define texel_t struct aiTexel
#define vector2_t struct aiVector2D
#define vector3_t struct aiVector3D
#define color3_t struct aiColor3D
#define color4_t struct aiColor4D
#define quaternion_t struct aiQuaternion
#define matrix3_t struct aiMatrix3x3
#define matrix4_t struct aiMatrix4x4
#define aistring_t struct aiString

/* Userdata memory */
#define ud_t moonassimp_ud_t
typedef struct {
    void *obj; /* the object bound to this userdata */
    uint32_t marks;
    scene_t *scene; /* the scene it belongs to */
    mesh_t *mesh;   /* the mesh it belongs to (NULL if not applicable) */
    material_t *material; /* the material it belongs to (NULL if not applicable) */
    animation_t *animation; /* the animation it belongs to (NULL if not applicable) */
} moonassimp_ud_t;

/* Marks.  m_ = marks word (uint32_t) , i_ = bit number (0 .. 31)  */
#define MarkGet(m_,i_)  (((m_) & ((uint32_t)1<<(i_))) == ((uint32_t)1<<(i_)))
#define MarkSet(m_,i_)  do { (m_) = ((m_) | ((uint32_t)1<<(i_))); } while(0)
#define MarkReset(m_,i_) do { (m_) = ((m_) & (~((uint32_t)1<<(i_)))); } while(0)

#define IsValid(ud)             MarkGet((ud)->marks, 0)
#define MarkValid(ud)           MarkSet((ud)->marks, 0) 
#define CancelValid(ud)         MarkReset((ud)->marks, 0)

#if 0
/* .c */
#define  moonassimp_
#endif

#define testxxx moonassimp_testxxx
void* testxxx(lua_State *L, int arg, const char *mt);
#define checkxxx moonassimp_checkxxx
void* checkxxx(lua_State *L, int arg, const char *mt);
#define pushxxx moonassimp_pushxxx
int pushxxx(lua_State *L, void *p);

#define newuserdata moonassimp_newuserdata
ud_t *newuserdata(lua_State *L, void *ptr, const char *mt);
#define freeuserdata moonassimp_freeuserdata
int freeuserdata(lua_State *L, void *ptr);
#define userdata(ptr) (ud_t*)udata_mem(ptr)
#define userdata_unref(L, ptr) udata_unref((L),(ptr))

/* animmesh.c */
#define newanimmesh moonassimp_newanimmesh
int newanimmesh(lua_State *L, scene_t *scene, mesh_t *mesh, animmesh_t *animmesh);
#define freeanimmesh moonassimp_freeanimmesh
int freeanimmesh(lua_State *L, animmesh_t *animmesh);
#define testanimmesh(L, arg) (animmesh_t*)testxxx((L), (arg), ANIMMESH_MT)
#define checkanimmesh(L, arg) (animmesh_t*)checkxxx((L), (arg), ANIMMESH_MT)
#define pushanimmesh(L, p) pushxxx((L), (p))

/* meshanim.c */
#define newmeshanim moonassimp_newmeshanim
int newmeshanim(lua_State *L, scene_t *scene, animation_t *animation, meshanim_t *meshanim);
#define freemeshanim moonassimp_freemeshanim
int freemeshanim(lua_State *L, meshanim_t *meshanim);
#define testmeshanim(L, arg) (meshanim_t*)testxxx((L), (arg), MESHANIM_MT)
#define checkmeshanim(L, arg) (meshanim_t*)checkxxx((L), (arg), MESHANIM_MT)
#define pushmeshanim(L, p) pushxxx((L), (p))

/* nodeanim.c */
#define newnodeanim moonassimp_newnodeanim
int newnodeanim(lua_State *L, scene_t *scene, animation_t *animation, nodeanim_t *nodeanim);
#define freenodeanim moonassimp_freenodeanim
int freenodeanim(lua_State *L, nodeanim_t *nodeanim);
#define testnodeanim(L, arg) (nodeanim_t*)testxxx((L), (arg), NODEANIM_MT)
#define checknodeanim(L, arg) (nodeanim_t*)checkxxx((L), (arg), NODEANIM_MT)
#define pushnodeanim(L, p) pushxxx((L), (p))

/* face.c */
#define newface moonassimp_newface
int newface(lua_State *L, scene_t *scene, mesh_t *mesh, face_t *face);
#define freeface moonassimp_freeface
int freeface(lua_State *L, face_t *face);
#define testface(L, arg) (face_t*)testxxx((L), (arg), FACE_MT)
#define checkface(L, arg) (face_t*)checkxxx((L), (arg), FACE_MT)
#define pushface(L, p) pushxxx((L), (p))

/* bone.c */
#define newbone moonassimp_newbone
int newbone(lua_State *L, scene_t *scene, mesh_t *mesh, bone_t *bone);
#define freebone moonassimp_freebone
int freebone(lua_State *L, bone_t *bone);
#define testbone(L, arg) (bone_t*)testxxx((L), (arg), BONE_MT)
#define checkbone(L, arg) (bone_t*)checkxxx((L), (arg), BONE_MT)
#define pushbone(L, p) pushxxx((L), (p))

/* animation.c */
#define newanimation moonassimp_newanimation
int newanimation(lua_State *L, scene_t *scene, animation_t *animation);
#define freeanimation moonassimp_freeanimation
int freeanimation(lua_State *L, animation_t *animation);
#define testanimation(L, arg) (animation_t*)testxxx((L), (arg), ANIMATION_MT)
#define checkanimation(L, arg) (animation_t*)checkxxx((L), (arg), ANIMATION_MT)
#define pushanimation(L, p) pushxxx((L), (p))

/* texture.c */
#define newtexture moonassimp_newtexture
int newtexture(lua_State *L, scene_t *scene, texture_t *texture);
#define freetexture moonassimp_freetexture
int freetexture(lua_State *L, texture_t *texture);
#define testtexture(L, arg) (texture_t*)testxxx((L), (arg), TEXTURE_MT)
#define checktexture(L, arg) (texture_t*)checkxxx((L), (arg), TEXTURE_MT)
#define pushtexture(L, p) pushxxx((L), (p))

/* light.c */
#define newlight moonassimp_newlight
int newlight(lua_State *L, scene_t *scene, light_t *light);
#define freelight moonassimp_freelight
int freelight(lua_State *L, light_t *light);
#define testlight(L, arg) (light_t*)testxxx((L), (arg), LIGHT_MT)
#define checklight(L, arg) (light_t*)checkxxx((L), (arg), LIGHT_MT)
#define pushlight(L, p) pushxxx((L), (p))

/* camera.c */
#define newcamera moonassimp_newcamera
int newcamera(lua_State *L, scene_t *scene, camera_t *camera);
#define freecamera moonassimp_freecamera
int freecamera(lua_State *L, camera_t *camera);
#define testcamera(L, arg) (camera_t*)testxxx((L), (arg), CAMERA_MT)
#define checkcamera(L, arg) (camera_t*)checkxxx((L), (arg), CAMERA_MT)
#define pushcamera(L, p) pushxxx((L), (p))

/* material.c */
#define newmaterial moonassimp_newmaterial
int newmaterial(lua_State *L, scene_t *scene, material_t *material);
#define freematerial moonassimp_freematerial
int freematerial(lua_State *L, material_t *material);
#define testmaterial(L, arg) (material_t*)testxxx((L), (arg), MATERIAL_MT)
#define checkmaterial(L, arg) (material_t*)checkxxx((L), (arg), MATERIAL_MT)
#define pushmaterial(L, p) pushxxx((L), (p))

/* mesh.c */
#define newmesh moonassimp_newmesh
int newmesh(lua_State *L, scene_t *scene, mesh_t *mesh);
#define freemesh moonassimp_freemesh
int freemesh(lua_State *L, mesh_t *mesh);
#define testmesh(L, arg) (mesh_t*)testxxx((L), (arg), MESH_MT)
#define checkmesh(L, arg) (mesh_t*)checkxxx((L), (arg), MESH_MT)
#define pushmesh(L, p) pushxxx((L), (p))

/* node.c */
#define newnode moonassimp_newnode
int newnode(lua_State *L, scene_t *scene, node_t *node);
#define freenode moonassimp_freenode
int freenode(lua_State *L, node_t *node);
#define testnode(L, arg) (node_t*)testxxx((L), (arg), NODE_MT)
#define checknode(L, arg) (node_t*)checkxxx((L), (arg), NODE_MT)
#define pushnode(L, p) pushxxx((L), (p))

/* scene.c */
#define newscene moonassimp_newscene
int newscene(lua_State *L, scene_t *scene);
#define testscene(L, arg) (scene_t*)testxxx((L), (arg), SCENE_MT)
#define checkscene(L, arg) (scene_t*)checkxxx((L), (arg), SCENE_MT)
#define pushscene(L, p) pushxxx((L), (p))

#if 0 /* scaffoldings */
#define testzzz(L, arg) (zzz_t*)testxxx((L), (arg), ZZZ_MT)
#define checkzzz(L, arg) (zzz_t*)checkxxx((L), (arg), ZZZ_MT)
#define pushzzz(L, p) pushxxx((L), (p))
#endif

#endif /* objectsDEFINED */
