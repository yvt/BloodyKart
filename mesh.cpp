/*
 *  mesh.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/23.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */
#include <array>
#include "global.h"
#include "mesh.h"
#include "glpng.h"
#include <sys/stat.h>
#include "map.h"

#if GL_ARB_shader_objects
static std::array<GLhandleARB, 4> prg_bump1;
static GLhandleARB prg_bump2;
static GLhandleARB prg_bump3;
#endif

static int attr_tU=0;
static int attr_tV=1;
static int attr_gIvt=2;
static int attr_sV=3;

void xmaterial::begin(bool gi, bool is_fastlight, bool allowGLSL, bool lit1){
	glColor4f(dr,dg,db,da);
	if(texed){
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex);
	}else
		glDisable(GL_TEXTURE_2D);
	
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
				 color4f(dr, dg, db, da));
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
				 color4f(sr, sg, sb, 1.f));
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, power);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
				 color4f(er, eg, eb, 0.f));
	
	if(use_glsl){
		if(glGetHandleARB(GL_PROGRAM_OBJECT_ARB) || (!allowGLSL))
			return;
	}else{
		if(!allowGLSL)
			return;
	}
	if(!is_fastlight){
		int shader_index = (bump ? 1 : 0) | (texed ? 2 : 0);
		GLhandleARB gl_program = prg_bump1[shader_index];
		if(bump && use_glsl){
			glActiveTexture(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, bump);
			glActiveTexture(GL_TEXTURE0_ARB);
			glUseProgramObjectARB(gl_program);
			glUniform1iARB(glGetUniformLocationARB(gl_program, "tex"),
						   0);
			glUniform1iARB(glGetUniformLocationARB(gl_program, "bump"),
						   1);
			glUniform1iARB(glGetUniformLocationARB(gl_program, "isNormalMap"),
						   1);
			glUniform1iARB(glGetUniformLocationARB(gl_program, "bumped"),
						   1);
			glUniform1fARB(glGetUniformLocationARB(gl_program, "scale"),
						   1.f);
			glUniform1iARB(glGetUniformLocationARB(gl_program, "gIed"),
						   gi?1:0);
			glUniform2fARB(glGetUniformLocationARB(gl_program, "bumpSizeInv"),
						   1.f/(float)bW, 1.f/(float)bH);
			
		}else if(use_glsl){
			glUseProgramObjectARB(gl_program);
			glUniform1iARB(glGetUniformLocationARB(gl_program, "tex"),
						   0);
			glUniform1iARB(glGetUniformLocationARB(gl_program, "bump"),
						   0);
			glUniform1iARB(glGetUniformLocationARB(gl_program, "bumped"),
						   0);
			glUniform1iARB(glGetUniformLocationARB(gl_program, "gIed"),
						   gi?1:0);
		}
		if(use_glsl){
			glUniform1iARB(glGetUniformLocationARB(gl_program, "texed"),
						   texed?1:0);
			glUniform1iARB(glGetUniformLocationARB(gl_program, "giTex"),
						   2);
			
		}
	}else{
		GLhandleARB shd=prg_bump2;
		if(bump && use_glsl){
			glActiveTexture(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, bump);
			glActiveTexture(GL_TEXTURE0_ARB);
			if(lit1)
				shd=prg_bump3;
			glUseProgramObjectARB(shd);
			glUniform1iARB(glGetUniformLocationARB(shd, "tex"),
						   0);
			glUniform1iARB(glGetUniformLocationARB(shd, "bump"),
						   1);
			glUniform1iARB(glGetUniformLocationARB(shd, "isNormalMap"),
						   1);
			glUniform1iARB(glGetUniformLocationARB(shd, "bumped"),
						   1);
			glUniform1fARB(glGetUniformLocationARB(shd, "scale"),
						   1.f);
			glUniform1iARB(glGetUniformLocationARB(shd, "gIed"),
						   gi?1:0);
			glUniform2fARB(glGetUniformLocationARB(shd, "bumpSizeInv"),
						   1.f/(float)bW, 1.f/(float)bH);
		}else if(use_glsl){
			glUseProgramObjectARB(shd);
			glUniform1iARB(glGetUniformLocationARB(shd, "tex"),
						   0);
			glUniform1iARB(glGetUniformLocationARB(shd, "bump"),
						   0);
			glUniform1iARB(glGetUniformLocationARB(shd, "bumped"),
						   0);
			glUniform1iARB(glGetUniformLocationARB(shd, "gIed"),
						   gi?1:0);
		}
		if(use_glsl){
			glUniform1iARB(glGetUniformLocationARB(shd, "texed"),
						   texed?1:0);
			glUniform1iARB(glGetUniformLocationARB(shd, "giTex"),
						   2);
			
			if(lit1){
				glUniform1iARB(glGetUniformLocationARB(shd, "giTex1"),
							   3);
				glUniform1iARB(glGetUniformLocationARB(shd, "giTex2"),
							   4);
				glUniform1iARB(glGetUniformLocationARB(shd, "giTex3"),
							   5);
				glUniform1iARB(glGetUniformLocationARB(shd, "giTex4"),
							   6);
			}
		}
	}
	
}
void xmaterial::end(){
	glEnable(GL_TEXTURE_2D);
	//glEnable(GL_LIGHTING);
#if GL_ARB_shader_objects
	if(use_glsl)
		glUseProgramObjectARB(0);
#endif
}

void mesh::render(int phase){
	
#if GL_ARB_shader_objects
	if(use_glsl){
		GLhandleARB ohand=glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
		for (auto &p: prg_bump1) {
			glUseProgramObjectARB(p);
			glUniform3fvARB(glGetUniformLocationARB(p, "ldV00"), 4,
							(GLfloat *)&(curLight.v00));
			glUniform3fvARB(glGetUniformLocationARB(p, "ldV10"), 4,
							(GLfloat *)&(curLight.v10));
			glUniform3fvARB(glGetUniformLocationARB(p, "ldV11"), 4,
							(GLfloat *)&(curLight.v11));
			glUniform3fvARB(glGetUniformLocationARB(p, "ldV12"), 4,
							(GLfloat *)&(curLight.v12));
		}
		glUseProgramObjectARB(ohand);
	}
#endif
	
	if(giGLSL!=use_glsl){
		if(giTex){
			updateGi();
		}
	}
	glFrontFace(g_currentlyRenderingBack?GL_CW
				:GL_CCW);
	
	int n;
	
	if(phase==-1){
		for(n=0;n<count_material;n++)
			totalPolys+=matPolys[n];
	}else{
		totalPolys+=matPolys[phase];
	}
	if(list_mesh[phase+1] && ouse_glsl[phase+1]==use_glsl && invalid[phase+1]==false){
		glCallList(list_mesh[phase+1]);
		return;
	}
	if(list_mesh[phase+1]==0)
		list_mesh[phase+1]=glGenLists(1);
	invalid[phase+1]=false;
	ouse_glsl[phase+1]=use_glsl;
	glNewList(list_mesh[phase+1], GL_COMPILE_AND_EXECUTE);
	
	long omat, ind, i;


	glEnable(GL_CULL_FACE);
	
	glCullFace(GL_FRONT);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	omat=-1;
	glBegin(GL_TRIANGLES);
#if GL_ARB_shader_objects
	if(use_glsl){
		glVertexAttrib3fARB(attr_gIvt, 0.f, 0.f, 0.f);
		glVertexAttrib1fARB(attr_sV, 1.f);
	}
#endif
		
	for(n=0;n<count_face;n++){
		if(phase==-1 || phase==face[n*4+3]){
			if(face[n*4+3]!=omat){
				
				glEnd();
				if(omat>=0)
					mat[omat].end();
				omat=face[n*4+3];
				
				
				
#if GL_ARB_shader_objects && GL_ARB_multitexture
				// set gi
				if(use_glsl){
					if(giTex && !glGetHandleARB(GL_PROGRAM_OBJECT_ARB)){
						glActiveTexture(GL_TEXTURE2_ARB);
						glBindTexture(GL_TEXTURE_2D, giTex);
#if GL_TEXTURE_MAX_ANISOTROPY_EXT
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
						if(lit1){
							glActiveTexture(GL_TEXTURE3_ARB);
							glBindTexture(GL_TEXTURE_2D, giTex1);
#if GL_TEXTURE_MAX_ANISOTROPY_EXT
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
							glActiveTexture(GL_TEXTURE4_ARB);
							glBindTexture(GL_TEXTURE_2D, giTex2);
#if GL_TEXTURE_MAX_ANISOTROPY_EXT
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
							glActiveTexture(GL_TEXTURE5_ARB);
							glBindTexture(GL_TEXTURE_2D, giTex3);
#if GL_TEXTURE_MAX_ANISOTROPY_EXT
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
							glActiveTexture(GL_TEXTURE6_ARB);
							glBindTexture(GL_TEXTURE_2D, giTex4);
#if GL_TEXTURE_MAX_ANISOTROPY_EXT
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
						}
						glActiveTexture(GL_TEXTURE0_ARB);
					}
				}else
#endif
					
#if GL_ARB_multitexture
				if(cap_multiTex){
					
					
					glActiveTextureARB(GL_TEXTURE1_ARB);
					glDisable(GL_TEXTURE_2D);
					glActiveTextureARB(GL_TEXTURE0_ARB);
					
					if(giTex){
						if(mat[omat].texed){
							glActiveTextureARB(GL_TEXTURE1_ARB);
							glEnable(GL_TEXTURE_2D);
							glBindTexture(GL_TEXTURE_2D, giTex);
							glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB,		GL_PREVIOUS);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA,	GL_PREVIOUS);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB,		GL_TEXTURE0_ARB);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA,	GL_TEXTURE0_ARB);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB,		GL_SRC_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA,	GL_SRC_ALPHA);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB,		GL_SRC_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA,	GL_SRC_ALPHA);
							glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB,	GL_MODULATE);
							glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA,	GL_MODULATE);
							glActiveTextureARB(GL_TEXTURE0_ARB);
							glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB,		GL_PRIMARY_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA,	GL_PRIMARY_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB,		GL_TEXTURE1_ARB);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA,	GL_TEXTURE1_ARB);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB,		GL_SRC_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA,	GL_SRC_ALPHA);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB,		GL_SRC_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA,	GL_SRC_ALPHA);
							glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB,	GL_ADD);
							glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA,	GL_REPLACE);
						}else{
						
							glBindTexture(GL_TEXTURE_2D, giTex);
							
							glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB,		GL_PRIMARY_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA,	GL_PRIMARY_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB,		GL_TEXTURE0_ARB);
							glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA,	GL_TEXTURE0_ARB);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB,		GL_SRC_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA,	GL_SRC_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB,		GL_SRC_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA,	GL_SRC_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB,	GL_ADD);
							glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA,	GL_REPLACE);
						}
					}else{
						glActiveTextureARB(GL_TEXTURE1_ARB);
						glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
						glActiveTextureARB(GL_TEXTURE0_ARB);
						glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
					}
				}
#else
				{}
#endif
				
				
				mat[omat].begin(gi, is_fastlight, allowGLSL, lit1);
#if GL_ARB_multitexture
				if(cap_multiTex){
					if(giTex){
						glEnable(GL_TEXTURE_2D);
					}
				}
#endif

				glBegin(GL_TRIANGLES);
			}
			
			for(i=0;i<3;i++){
				ind=face[n*4+i];
				if(uv){
					glTexCoord2f(uv[ind].x, uv[ind].y);
#if GL_ARB_shader_objects
					if(use_glsl){
						glVertexAttrib3fARB(attr_tU, uu[ind].x, uu[ind].y, uu[ind].z);
						glVertexAttrib3fARB(attr_tV, vv[ind].x, vv[ind].y, vv[ind].z);
					}
#endif
				}
				if(gi && cap_multiTex){
#if GL_ARB_shader_objects && GL_ARB_multitexture
					if(use_glsl){
						glMultiTexCoord2fARB(GL_TEXTURE2_ARB, giUV[(n*3)+i].x, giUV[(n*3)+i].y);
					}else{
#endif
#if GL_ARB_multitexture
						if(cap_multiTex){
							if(mat[omat].texed){
								glMultiTexCoord2fARB(GL_TEXTURE1_ARB, giUV[(n*3)+i].x, giUV[(n*3)+i].y);
							}else{
								glMultiTexCoord2fARB(GL_TEXTURE0_ARB, giUV[(n*3)+i].x, giUV[(n*3)+i].y);

							}
						}
#endif
						/*glColor4f(mat[omat].dr+gi[ind].x, mat[omat].dg+gi[ind].y, mat[omat].db+gi[ind].z,
								  mat[omat].da);*/
#if GL_ARB_shader_objects && GL_ARB_multitexture
					}
#endif
				}
#if GL_ARB_shader_objects && GL_ARB_multitexture
				if(shadow && use_glsl){
					glVertexAttrib1fARB(attr_sV, shadow[ind]);
				}
#endif
				
				glNormal3f(normal[ind].x, normal[ind].y, normal[ind].z);
				glVertex3f(vertex[ind].x, vertex[ind].y, vertex[ind].z);
			}
		}
	}
	glEnd();
	
	if(omat>=0)
		mat[omat].end();
	glDisable(GL_CULL_FACE);
#if GL_ARB_multitexture
	
	if(cap_multiTex){
		
		// reset texture env
		
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color4f(0.f, 0.f, 0.f, 0.f));
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		
		
	}
	
#endif
	glEndList();
	

}
void process_path(char *buf){
	char tmp[256];
	strcpy(tmp, buf);
	sprintf(buf, "res/%s", tmp);
}
mesh::~mesh(){
	int n;
	if(rayCache)
		delete[] rayCache;
	if(vertex)
		delete[] vertex;
	if(normal)
		delete[] normal;
	if(uv)
		delete[] uv;
	if(uu)
		delete[] uu;
	if(vv)
		delete[] vv;
	if(gi)
		delete[] gi;
	if(giShadow)
		delete[] giShadow;
	if(lit)
		delete[] lit;
	if(shadow)
		delete[] shadow;
	if(cnt)
		delete[] cnt;
	if(face)
		delete[] face;
	if(oVertex)
		delete[] oVertex;
	if(oNormal)
		delete[] oNormal;
	if(oUU)
		delete[] oUU;
	if(oVV)
		delete[] oVV;
	if(mat){
		for(n=0;n<count_material;n++){
			if(mat[n].texed){
				glDeleteTextures(1, &(mat[n].tex));
				
			}
			if(mat[n].bump){
				glDeleteTextures(1, &(mat[n].bump));
			}
		}
		delete[] mat;
	}
	if(giTex)
		glDeleteTextures(1, &giTex);
	if(giTex1)
		glDeleteTextures(1, &giTex1);
	if(giTex2)
		glDeleteTextures(1, &giTex2);
	if(giTex3)
		glDeleteTextures(1, &giTex3);
	if(giTex4)
		glDeleteTextures(1, &giTex4);
	for(n=0;n<MAX_MATERIALS;n++){
		if(list_mesh[n]){
			glDeleteLists(list_mesh[n], 1);
		}
	}
	for(n=0;n<count_bone;n++)
		delete[] boneW[n];
}
mesh::mesh(XFILE f){
	loadMesh(f, true);
}
mesh::mesh(const char *fn){
	XFILE f;
	f=xparserOpen((fn));
	if(f==NULL){
		throw "mesh: can't open mesh";
	}
	loadMesh(f);
	xparserClose(f);
}
void mesh::loadMesh(XFILE f, bool strRead){
	
	char st[256];
	long n, i;
	int clos;
	
	oVertex=NULL;
	oNormal=NULL;
	oUU=NULL;
	oVV=NULL;
	uv=NULL;
	mat=NULL;
	gi=NULL;
	giShadow=NULL;
	giTex=giTex1=giTex2=giTex3=giTex4=NULL;
	lit=NULL; // Light Map Front
	lit1=lit2=NULL; // Light Map Side
	lit3=lit4=NULL;
	shadow=NULL;
	count_material=1;
	count_bone=0;
	count_vertex=0;
	mat=new xmaterial[1];
	mat[0].dr=1;
	mat[0].dg=1;
	mat[0].db=1;
	mat[0].da=1;
	mat[0].texed=false;
	giTex=0;
	is_fastlight=false;
	allowGLSL=true;
	memset(list_mesh ,0, sizeof(list_mesh));
	memset(matPolys, 0, sizeof(matPolys));
	rayCache=NULL;
	for(n=0;n<MAX_BONES;n++)
		identityMatrix4(boneM[n]);
	while(1){
		if(strRead && count_vertex==0){
			strRead=false;
			goto fst;
		}
		if(xparserNodeType(f)!=XNODE_DATA)break;
		xparserBeginData(f);
		xparserReadID(st, f);
		clos=1;
		//fprintf(stderr, "ID=%s\n", st);
		if(strcmp(st, "Mesh")==0){
		fst:
			clos=0;
			count_vertex=(long)xparserReadValue(f);
			vertex=new VECTOR[count_vertex];
			normal=new VECTOR[count_vertex];
			
			uu=new VECTOR[count_vertex];
			vv=new VECTOR[count_vertex];
			cnt=new int[count_vertex];
			
			for(n=0;n<count_vertex;n++){
				vertex[n].x=xparserReadValue(f);
				vertex[n].y=xparserReadValue(f);
				vertex[n].z=xparserReadValue(f);
				normal[n].x=0;
				normal[n].y=0;
				normal[n].z=0;
				uu[n]=vv[n]=vec3_t(0.f, 0.f, 0.f);
				cnt[n]=0;
			}
			count_face=(long)xparserReadValue(f);
			face=new long[count_face*4];
			for(n=0;n<count_face;n++){
				i=(long)xparserReadValue(f);
				face[n*4]=(long)xparserReadValue(f);
				face[n*4+1]=(long)xparserReadValue(f);
				face[n*4+2]=(long)xparserReadValue(f);
				face[n*4+3]=0;
				while(i>3){
					xparserReadValue(f);
					i--;
				}
				
			}
		}else if(strcmp(st, "MeshNormals")==0){
			xparserReadValue(f);
			for(n=0;n<count_vertex;n++){
				normal[n].x=xparserReadValue(f);
				normal[n].y=xparserReadValue(f);
				normal[n].z=xparserReadValue(f);
			}
		}else if(strcmp(st, "MeshTextureCoords")==0){
			xparserReadValue(f);
			uv=new VECTOR[count_vertex];
			for(n=0;n<count_vertex;n++){
				uv[n].x=xparserReadValue(f);
				uv[n].y=xparserReadValue(f);
				//printf("%f %f\n", uv[n].x, uv[n].y);
			}
		}else if(strcmp(st, "SkinWeights")==0){
			int i=count_bone;
			xparserReadString(st, f);
			if(st[0]=='"'){
				st[strlen(st)-1]=0;
				boneName[i]=st+1;
			}else{
				boneName[i]=st;
			}
			
			int cnt=(int)xparserReadValue(f);
			int *indBuf;
			
			if(!oVertex){
				// create unskinned vertices
				oVertex=new VECTOR[count_vertex];
				oNormal=new VECTOR[count_vertex];
				oUU=new VECTOR[count_vertex];
				oVV=new VECTOR[count_vertex];
				memcpy(oVertex, vertex, sizeof(vec3_t)*count_vertex);
				memcpy(oNormal, normal, sizeof(vec3_t)*count_vertex);
				// oUU & oVV is not calculated yet, not memcpying
			}
			
			boneW[i]=new float[count_vertex];
			indBuf=new int[cnt];
			memset(boneW[i], 0, count_vertex*sizeof(float));
			
			// read skin index buffer
			for(n=0;n<cnt;n++){
				indBuf[n]=(int)xparserReadValue(f);
			}
			
			// read skin weight
			for(n=0;n<cnt;n++){
				boneW[i][indBuf[n]]=(float)xparserReadValue(f);
			}
			
			// read skin offset
			
			for(n=0;n<16;n++)
				boneOff[i][n]=(float)xparserReadValue(f);
			
			count_bone++;
			
			delete[] indBuf;
			
		}else if(strcmp(st, "MeshMaterialList")==0){
			count_material=(long)xparserReadValue(f);
			mat=new xmaterial[count_material];
			xparserReadValue(f);
			for(n=0;n<count_face;n++){
				face[n*4+3]=(long)xparserReadValue(f);
				matPolys[face[n*4+3]]++;
			}
			for(n=0;n<count_material;n++){
				xparserBeginData(f);
				xparserReadID(st, f);
				memset(&mat[n], 0, sizeof(xmaterial));
				strcpy(mat[n].name, st);
				//fprintf(stderr, "ID=%s\n", st);
				mat[n].dr=xparserReadValue(f);
				mat[n].dg=xparserReadValue(f);
				mat[n].db=xparserReadValue(f);
				mat[n].da=xparserReadValue(f);
				
				mat[n].power=xparserReadValue(f);
				
				mat[n].sr=xparserReadValue(f);
				mat[n].sg=xparserReadValue(f);
				mat[n].sb=xparserReadValue(f);
				
				mat[n].er=xparserReadValue(f);
				mat[n].eg=xparserReadValue(f);
				mat[n].eb=xparserReadValue(f);
				
				mat[n].texed=false;
				
				mat[n].bump=0;
				
				mat[n].ar=mat[n].ag=mat[n].ab=
				mat[n].aa=1.f;
				//puts("cp1");
				if(xparserNodeType(f)==XNODE_DATA){	
					
					xparserBeginData(f);
					xparserReadString(st, f);
					glGenTextures(1, &mat[n].tex);
					glBindTexture(GL_TEXTURE_2D, mat[n].tex);
					
					mat[n].texed=true;
					process_path(st);
					glpngLoadTexture(st, true, false, &mat[n].ar, &mat[n].ag, &mat[n].ab, &mat[n].aa);
					//printf("Load Texture: %s\n", st);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TEXTURE_FILTER);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TEXTURE_FILTER);
#if GL_TEXTURE_MAX_ANISOTROPY_EXT
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
#endif
					
					
					char ext[64];
					strcpy(ext, strrchr(st, '.'));
					strcpy(strrchr(st, '.'), ".bump");
					strcat(st, ext);
					
					struct stat ss;
					if(cap_glsl && !stat(st, &ss)){
						// there is bumpmap
						glGenTextures(1, &mat[n].bump);
						glBindTexture(GL_TEXTURE_2D, mat[n].bump);
						//process_path(st);
						//printf("Load Bump: %s\n", st);
						glpngLoadTexture(st, true, true, NULL, NULL, NULL, NULL, &mat[n].bW, &mat[n].bH);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
#if GL_TEXTURE_MAX_ANISOTROPY_EXT
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
#endif
						
					}
					
					xparserEndData(f);
				}
				//puts("cp2");
				
				xparserEndData(f);
			}
		}
		if(clos)xparserEndData(f);
	}
	if(count_vertex==0)
		throw "no vertices in mesh";
	//printf("%ld vertex\n", count_vertex);
	//printf("%ld face\n", count_face);
	//printf("%ld material\n", count_material);
	//list_mesh[0]=list_mesh[1]=0;
	memset(list_mesh, 0, sizeof(list_mesh));
	//render();
	if(cap_glsl)
		calc_tangent();
}
static inline int ceilSqrt(int vl){
	return (int)ceil(sqrt((double)vl)+.0000001);
}
static inline unsigned char giTexVal(float v){
	int i;
	i=(int)(255.f*powf(v*(use_glsl?.5f:1.f), use_glsl?1.f:1.f)); // enhance dynamic range
	if(i<0)
		i=0;
	if(i>255)
		i=255;
	return (unsigned char)i;
}
static inline float giFTexVal(float v){
	float i;
	if(v<.00000001f)
		return 0.f;
	i=powf(v*(use_glsl?.5f:1.f), use_glsl?1.f:1.f); // enhance dynamic range
	return i;
}
void mesh::updateSkin(){
	// do skinning
	
	int n, i;
	if(count_bone==0)
		return; 
	
	for(n=0;n<count_vertex;n++){
		vec3_t p1, p2, p3;
		vec3_t n1, n2, n3;
		vec3_t u1, u2, u3;
		vec3_t v1, v2, v3;
		float *m;
		float wt=0.f;
		
		p1=oVertex[n]; n1=oNormal[n];
		u1=oUU[n]; v1=oVV[n];
		p2=n2=u2=v2=vec3_t(0.f, 0.f, 0.f);
		for(i=0;i<count_bone;i++){
			m=boneM[i];
			if(boneW[i][n]<.00001f)
				continue; // no influation
			
			// multiple position
			p3.x=p1.x*m[0]+p1.y*m[1]+p1.z*m[2]+m[3];
			p3.y=p1.x*m[4]+p1.y*m[5]+p1.z*m[6]+m[7];
			p3.z=p1.x*m[8]+p1.y*m[9]+p1.z*m[10]+m[11];
			
			// multiple normal
			n3.x=n1.x*m[0]+n1.y*m[1]+n1.z*m[2];
			n3.y=n1.x*m[4]+n1.y*m[5]+n1.z*m[6];
			n3.z=n1.x*m[8]+n1.y*m[9]+n1.z*m[10];
			
			// multiple tangent U
			u3.x=u1.x*m[0]+u1.y*m[1]+u1.z*m[2];
			u3.y=u1.x*m[4]+u1.y*m[5]+u1.z*m[6];
			u3.z=u1.x*m[8]+u1.y*m[9]+u1.z*m[10];
			
			// multiple tangent V
			v3.x=v1.x*m[0]+v1.y*m[1]+v1.z*m[2];
			v3.y=v1.x*m[4]+v1.y*m[5]+v1.z*m[6];
			v3.z=v1.x*m[8]+v1.y*m[9]+v1.z*m[10];
			
			p2+=p3*boneW[i][n];
			n2+=n3*boneW[i][n];
			u2+=u3*boneW[i][n];
			v2+=v3*boneW[i][n];
			wt+=boneW[i][n];
		}
		/*if(wt<.00001f){
			p2=p1; // no affecting skin
			n2=n1;
			u2=u1;
			v2=v1;
		}else{
			p2*=1.f/wt; // normalize
			n2*=1.f/wt;
			u2*=1.f/wt;
			v2*=1.f/wt;
		}*/
		
		vertex[n]=p2;
		normal[n]=n2;
		uu[n]=u2;
		vv[n]=v2;
		
		
	}
	
}
void mesh::updateGi(bool useLit){
	
	int n;
	n=ceilSqrt(count_face)*giDiv;
	giSize=1;
	
	// make it power of 2
	
	while(giSize<n)
		giSize<<=1;
	
	GLuint *texs[]={&giTex, &giTex1, &giTex2, &giTex3, &giTex4};
	
	for(GLuint **it2=texs;it2!=texs+5;it2++){
		GLuint *it=*it2;
		if(!(*it)){
	#if GL_ARB_texture_float
			if(use_hdr){
				glGenTextures(1, it);
				giFTexBuf=new float[giSize*giSize*4];
				giUV=new vec3_t[count_face*3];
				glBindTexture(GL_TEXTURE_2D, *it);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, giSize, giSize, 0, GL_RGBA, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}else{
	#endif
				glGenTextures(1, it);
				giTexBuf=new unsigned char[giSize*giSize*4];
				giUV=new vec3_t[count_face*3];
				glBindTexture(GL_TEXTURE_2D, *it);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, giSize, giSize, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	#if GL_ARB_texture_float
			}
	#endif
			
		}
		if(it2==texs && !lit1){
			// no side GI, aborting
			break;
		}
		
	}
	//memset(giTexBuf, rand(), giSize*giSize*4);
	int tx=0, ty=0;
	int ind;
	int xx, yy;
	float uvScale=1.f/(float)giSize;
	vec3_t uvShift=vec3_t(.5f, .5f, 0.f);
	//uvScale=1.f;
	
	for(n=0;n<count_face;n++){
		
		int ptr1, ptr2;
		
		ptr1=(ty*giSize+tx)<<2;
		ind=n*giDiv*giDiv;
		for(yy=0;yy<giDiv;yy++){
			ptr2=ptr1+((giSize*yy)<<2);
			for(xx=0;xx<giDiv;xx++){
				if(xx==yy+1){
					gi[ind]=gi[ind-1]+gi[ind+giDiv]-gi[ind-1+giDiv];
					if(lit)
						lit[ind]=lit[ind-1]+lit[ind+giDiv]-lit[ind-1+giDiv];
					giShadow[ind]=giShadow[ind-1]+giShadow[ind+giDiv]-giShadow[ind-1+giDiv];
				}
				if(use_hdr){
					if(lit && useLit) {
						
						giFTexBuf[ptr2++]=giFTexVal(gi[ind].x+lit[ind].x);
						giFTexBuf[ptr2++]=giFTexVal(gi[ind].y+lit[ind].y);
						giFTexBuf[ptr2++]=giFTexVal(gi[ind].z+lit[ind].z);
						giFTexBuf[ptr2++]=giFTexVal(giShadow[ind]);
						
					}else{
						giFTexBuf[ptr2++]=giFTexVal(gi[ind].x);
						giFTexBuf[ptr2++]=giFTexVal(gi[ind].y);
						giFTexBuf[ptr2++]=giFTexVal(gi[ind].z);
						giFTexBuf[ptr2++]=giFTexVal(giShadow[ind]);
						
					}
				}else{
					if(lit && useLit) {
						giTexBuf[ptr2++]=giTexVal(giShadow[ind]);
						giTexBuf[ptr2++]=giTexVal(gi[ind].z+lit[ind].z);
						giTexBuf[ptr2++]=giTexVal(gi[ind].y+lit[ind].y);
						giTexBuf[ptr2++]=giTexVal(gi[ind].x+lit[ind].x);
					}else{
						giTexBuf[ptr2++]=giTexVal(giShadow[ind]);
						giTexBuf[ptr2++]=giTexVal(gi[ind].z);
						giTexBuf[ptr2++]=giTexVal(gi[ind].y);
						giTexBuf[ptr2++]=giTexVal(gi[ind].x);
					}
				}
				ind++;
			}
		}
		
		giUV[(n*3)+0]=(vec3_t(tx, ty, 0)+uvShift)*uvScale;
		giUV[(n*3)+1]=(vec3_t(tx, ty+giDiv-1, 0)+uvShift)*uvScale;
		giUV[(n*3)+2]=(vec3_t(tx+giDiv-1, ty+giDiv-1, 0)+uvShift)*uvScale;
		
		tx+=giDiv;
		if(tx==giSize){
			tx=0; ty+=giDiv;
		}
	}
	
	glBindTexture(GL_TEXTURE_2D, giTex);
#if GL_ARB_texture_float
	if(use_hdr)
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, giSize, giSize, GL_RGBA, GL_FLOAT, giFTexBuf);
	else
#endif
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, giSize, giSize, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, giTexBuf);
	
	/* lit1 */
	
	if(lit1){
		tx=ty=0;
		for(n=0;n<count_face;n++){
			
			int ptr1, ptr2;
			
			ptr1=(ty*giSize+tx)<<2;
			ind=n*giDiv*giDiv;
			for(yy=0;yy<giDiv;yy++){
				ptr2=ptr1+((giSize*yy)<<2);
				for(xx=0;xx<giDiv;xx++){
					if(xx==yy+1){
						lit1[ind]=lit1[ind-1]+lit1[ind+giDiv]-lit1[ind-1+giDiv];
						
					}
					if(use_hdr){
						if(useLit) {
							
							giFTexBuf[ptr2++]=giFTexVal(lit1[ind].x);
							giFTexBuf[ptr2++]=giFTexVal(lit1[ind].y);
							giFTexBuf[ptr2++]=giFTexVal(lit1[ind].z);
							giFTexBuf[ptr2++]=giFTexVal(giShadow[ind]);
							
						}else{
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(giShadow[ind]);
							
						}
					}else{
						if(useLit) {
							giTexBuf[ptr2++]=giTexVal(giShadow[ind]);
							giTexBuf[ptr2++]=giTexVal(lit1[ind].z);
							giTexBuf[ptr2++]=giTexVal(lit1[ind].y);
							giTexBuf[ptr2++]=giTexVal(lit1[ind].x);
						}else{
							giTexBuf[ptr2++]=giTexVal(giShadow[ind]);
							giTexBuf[ptr2++]=giTexVal(0.f);
							giTexBuf[ptr2++]=giTexVal(0.f);
							giTexBuf[ptr2++]=giTexVal(0.f);
						}
					}
					ind++;
				}
			}
			
			giUV[(n*3)+0]=(vec3_t(tx, ty, 0)+uvShift)*uvScale;
			giUV[(n*3)+1]=(vec3_t(tx, ty+giDiv-1, 0)+uvShift)*uvScale;
			giUV[(n*3)+2]=(vec3_t(tx+giDiv-1, ty+giDiv-1, 0)+uvShift)*uvScale;
			
			tx+=giDiv;
			if(tx==giSize){
				tx=0; ty+=giDiv;
			}
		}
		
		glBindTexture(GL_TEXTURE_2D, giTex1);
#if GL_ARB_texture_float
		if(use_hdr)
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, giSize, giSize, GL_RGBA, GL_FLOAT, giFTexBuf);
		else
#endif
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, giSize, giSize, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, giTexBuf);
	}
	
	/* lit2 */
	
	if(lit2){
		tx=ty=0;
		for(n=0;n<count_face;n++){
			
			int ptr1, ptr2;
			
			ptr1=(ty*giSize+tx)<<2;
			ind=n*giDiv*giDiv;
			for(yy=0;yy<giDiv;yy++){
				ptr2=ptr1+((giSize*yy)<<2);
				for(xx=0;xx<giDiv;xx++){
					if(xx==yy+1){
						lit2[ind]=lit2[ind-1]+lit2[ind+giDiv]-lit2[ind-1+giDiv];
						
					}
					if(use_hdr){
						if(useLit) {
							
							giFTexBuf[ptr2++]=giFTexVal(lit2[ind].x);
							giFTexBuf[ptr2++]=giFTexVal(lit2[ind].y);
							giFTexBuf[ptr2++]=giFTexVal(lit2[ind].z);
							giFTexBuf[ptr2++]=giFTexVal(giShadow[ind]);
							
						}else{
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(giShadow[ind]);
							
						}
					}else{
						if(useLit) {
							giTexBuf[ptr2++]=giTexVal(giShadow[ind]);
							giTexBuf[ptr2++]=giTexVal(lit2[ind].z);
							giTexBuf[ptr2++]=giTexVal(lit2[ind].y);
							giTexBuf[ptr2++]=giTexVal(lit2[ind].x);
						}else{
							giTexBuf[ptr2++]=giTexVal(giShadow[ind]);
							giTexBuf[ptr2++]=giTexVal(0.f);
							giTexBuf[ptr2++]=giTexVal(0.f);
							giTexBuf[ptr2++]=giTexVal(0.f);
						}
					}
					ind++;
				}
			}
			
			giUV[(n*3)+0]=(vec3_t(tx, ty, 0)+uvShift)*uvScale;
			giUV[(n*3)+1]=(vec3_t(tx, ty+giDiv-1, 0)+uvShift)*uvScale;
			giUV[(n*3)+2]=(vec3_t(tx+giDiv-1, ty+giDiv-1, 0)+uvShift)*uvScale;
			
			tx+=giDiv;
			if(tx==giSize){
				tx=0; ty+=giDiv;
			}
		}
		
		glBindTexture(GL_TEXTURE_2D, giTex2);
#if GL_ARB_texture_float
		if(use_hdr)
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, giSize, giSize, GL_RGBA, GL_FLOAT, giFTexBuf);
		else
#endif
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, giSize, giSize, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, giTexBuf);
	}
	
	/* lit3 */
	
	if(lit3){
		tx=ty=0;
		for(n=0;n<count_face;n++){
			
			int ptr1, ptr2;
			
			ptr1=(ty*giSize+tx)<<2;
			ind=n*giDiv*giDiv;
			for(yy=0;yy<giDiv;yy++){
				ptr2=ptr1+((giSize*yy)<<2);
				for(xx=0;xx<giDiv;xx++){
					if(xx==yy+1){
						lit3[ind]=lit3[ind-1]+lit3[ind+giDiv]-lit3[ind-1+giDiv];
						
					}
					if(use_hdr){
						if(useLit) {
							
							giFTexBuf[ptr2++]=giFTexVal(lit3[ind].x);
							giFTexBuf[ptr2++]=giFTexVal(lit3[ind].y);
							giFTexBuf[ptr2++]=giFTexVal(lit3[ind].z);
							giFTexBuf[ptr2++]=giFTexVal(giShadow[ind]);
							
						}else{
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(giShadow[ind]);
							
						}
					}else{
						if(useLit) {
							giTexBuf[ptr2++]=giTexVal(giShadow[ind]);
							giTexBuf[ptr2++]=giTexVal(lit3[ind].z);
							giTexBuf[ptr2++]=giTexVal(lit3[ind].y);
							giTexBuf[ptr2++]=giTexVal(lit3[ind].x);
						}else{
							giTexBuf[ptr2++]=giTexVal(giShadow[ind]);
							giTexBuf[ptr2++]=giTexVal(0.f);
							giTexBuf[ptr2++]=giTexVal(0.f);
							giTexBuf[ptr2++]=giTexVal(0.f);
						}
					}
					ind++;
				}
			}
			
			giUV[(n*3)+0]=(vec3_t(tx, ty, 0)+uvShift)*uvScale;
			giUV[(n*3)+1]=(vec3_t(tx, ty+giDiv-1, 0)+uvShift)*uvScale;
			giUV[(n*3)+2]=(vec3_t(tx+giDiv-1, ty+giDiv-1, 0)+uvShift)*uvScale;
			
			tx+=giDiv;
			if(tx==giSize){
				tx=0; ty+=giDiv;
			}
		}
		
		glBindTexture(GL_TEXTURE_2D, giTex3);
#if GL_ARB_texture_float
		if(use_hdr)
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, giSize, giSize, GL_RGBA, GL_FLOAT, giFTexBuf);
		else
#endif
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, giSize, giSize, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, giTexBuf);
	}
	
	/* lit4 */
	
	if(lit4){
		tx=ty=0;
		for(n=0;n<count_face;n++){
			
			int ptr1, ptr2;
			
			ptr1=(ty*giSize+tx)<<2;
			ind=n*giDiv*giDiv;
			for(yy=0;yy<giDiv;yy++){
				ptr2=ptr1+((giSize*yy)<<2);
				for(xx=0;xx<giDiv;xx++){
					if(xx==yy+1){
						lit4[ind]=lit4[ind-1]+lit4[ind+giDiv]-lit4[ind-1+giDiv];
						
					}
					if(use_hdr){
						if(useLit) {
							
							giFTexBuf[ptr2++]=giFTexVal(lit4[ind].x);
							giFTexBuf[ptr2++]=giFTexVal(lit4[ind].y);
							giFTexBuf[ptr2++]=giFTexVal(lit4[ind].z);
							giFTexBuf[ptr2++]=giFTexVal(giShadow[ind]);
							
						}else{
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(0.f);
							giFTexBuf[ptr2++]=giFTexVal(giShadow[ind]);
							
						}
					}else{
						if(useLit) {
							giTexBuf[ptr2++]=giTexVal(giShadow[ind]);
							giTexBuf[ptr2++]=giTexVal(lit4[ind].z);
							giTexBuf[ptr2++]=giTexVal(lit4[ind].y);
							giTexBuf[ptr2++]=giTexVal(lit4[ind].x);
						}else{
							giTexBuf[ptr2++]=giTexVal(giShadow[ind]);
							giTexBuf[ptr2++]=giTexVal(0.f);
							giTexBuf[ptr2++]=giTexVal(0.f);
							giTexBuf[ptr2++]=giTexVal(0.f);
						}
					}
					ind++;
				}
			}
			
			giUV[(n*3)+0]=(vec3_t(tx, ty, 0)+uvShift)*uvScale;
			giUV[(n*3)+1]=(vec3_t(tx, ty+giDiv-1, 0)+uvShift)*uvScale;
			giUV[(n*3)+2]=(vec3_t(tx+giDiv-1, ty+giDiv-1, 0)+uvShift)*uvScale;
			
			tx+=giDiv;
			if(tx==giSize){
				tx=0; ty+=giDiv;
			}
		}
		
		glBindTexture(GL_TEXTURE_2D, giTex4);
#if GL_ARB_texture_float
		if(use_hdr)
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, giSize, giSize, GL_RGBA, GL_FLOAT, giFTexBuf);
		else
#endif
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, giSize, giSize, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, giTexBuf);
	}
	
	
	giGLSL=use_glsl;
	
}
void mesh::calc_tangent(){
	int n;
	if(uv==NULL)
		return;
	for(n=0;n<count_vertex;n++)
		uu[n]=vv[n]=vec3_t();
	for(n=0;n<count_face;n++){
		vec3_t v1, v2, v3;
		vec3_t t1, t2, t3;
		v1=vertex[face[(n<<2)]];
		v2=vertex[face[(n<<2)+1]];
		v3=vertex[face[(n<<2)+2]];
		t1=uv[face[(n<<2)]];
		t2=uv[face[(n<<2)+1]];
		t3=uv[face[(n<<2)+2]];
		//t2-=t1; t3-=t1; t1-=t1;
		//v2-=v1; v3-=v1; v1-=v1;
		
		vec3_t v;
		
		v=(v2-v1)*vec3_t::dot((t2-t1), vec3_t(1.f, 0.f, 0))
		+(v3-v1)*vec3_t::dot((t3-t1), vec3_t(1.f, 0.f, 0));
		v=v.normalize();
		uu[face[(n<<2)]+0]+=v;

		
		v=(v2-v1)*vec3_t::dot((t2-t1), vec3_t(0.f, 1.f, 0))
		+(v3-v1)*vec3_t::dot((t3-t1), vec3_t(0.f, 1.f, 0));
		v=v.normalize();
		vv[face[(n<<2)]+0]+=v;
		
		cnt[face[(n<<2)]+0]++;
		
		
		v=(v1-v2)*vec3_t::dot((t1-t2), vec3_t(1.f, 0.f, 0))
		+(v3-v2)*vec3_t::dot((t3-t2), vec3_t(1.f, 0.f, 0));
		v=v.normalize();
		uu[face[(n<<2)+1]]+=v;
		
		
		v=(v1-v2)*vec3_t::dot((t1-t2), vec3_t(0.f, 1.f, 0))
		+(v3-v2)*vec3_t::dot((t3-t2), vec3_t(0.f, 1.f, 0));
		v=v.normalize();
		vv[face[(n<<2)+1]]+=v;
		
		cnt[face[(n<<2)+1]]++;
		
		v=(v1-v3)*vec3_t::dot((t1-t3), vec3_t(1.f, 0.f, 0))
		+(v2-v3)*vec3_t::dot((t2-t3), vec3_t(1.f, 0.f, 0));
		v=v.normalize();
		uu[face[(n<<2)+2]]+=v;
		
		
		v=(v1-v3)*vec3_t::dot((t1-t3), vec3_t(0.f, 1.f, 0))
		+(v2-v3)*vec3_t::dot((t2-t3), vec3_t(0.f, 1.f, 0));
		v=v.normalize();
		vv[face[(n<<2)+2]]+=v;
		
		cnt[face[(n<<2)+2]]++;
		

	}
	
	for(n=0;n<count_vertex;n++){
		if(cnt[n]>1){
			float scl;
			scl=1.f/(float)cnt[n];
			uu[n]*=scl;
			vv[n]*=scl;
			//uu[n]=vec3_t(-1, 0, 0);
			//vv[n]=vec3_t(0, 0, 1);
		}
	}
	
	if(oUU){
		memcpy(oUU, uu, sizeof(vec3_t)*count_vertex);
	}
	if(oVV){
		memcpy(oVV, vv, sizeof(vec3_t)*count_vertex);
	}
}
vector<isect_t> *mesh::isect(vec3_t center, float radius){
	int n;;
	vector<isect_t> *hit2=new vector<isect_t>();
	vector<isect_t> &hits=*hit2;
	for(n=0;n<count_face;n++){
		vec3_t v1, v2, v3;
		float dp1, dp2, dp3, dp;
		
		v1=vertex[face[(n<<2)]];
		v2=vertex[face[(n<<2)+1]];
		v3=vertex[face[(n<<2)+2]];
		if(center.x<min(min(v1.x, v2.x), v3.x)-radius)
			continue;
		if(center.x>max(max(v1.x, v2.x), v3.x)+radius)
			continue;
		if(center.y<min(min(v1.y, v2.y), v3.y)-radius)
			continue;
		if(center.y>max(max(v1.y, v2.y), v3.y)+radius)
			continue;
		if(center.z<min(min(v1.z, v2.z), v3.z)-radius)
			continue;
		if(center.z>max(max(v1.z, v2.z), v3.z)+radius)
			continue;
		
		if(v1==v2 || v2==v3)
			continue;
		
		plane_t plane(v1, v2, v3);
		
		if((dp=fabs(plane.distance(center)))>radius)
			continue;
		if(plane.distance(center)<0.f)
			continue;
		
		
		
		
		vec3_t vx;
		plane_t planex;
		
		
		
		vx=plane.n+v1;
		planex=plane_t(v1, v2, vx);
		//assert(planex.distance(v3)<0.f);
		if((dp1=planex.distance(center))>radius)
			continue;
		
		vx=plane.n+v2;
		planex=plane_t(v2, v3, vx);
		//assert(planex.distance(v1)<0.f);
		if((dp2=planex.distance(center))>radius)
			continue;
		
		vx=plane.n+v3;
		planex=plane_t(v3, v1, vx);
		//assert(planex.distance(v2)<0.f);
		if((dp3=planex.distance(center))>radius)
			continue;
		
		if(dp1<=0 && dp2<=0 && dp3<=0){
			//face[n*4+3]=rand()%count_material;
			hits.push_back(isect_t(n, v1, v2, v3, dp));
			continue;
		}
		
		
		
		float r2=radius*radius-dp*dp;
		//
		if(vec3_t::dot(center-v1, v2-v1)>=0 && vec3_t::dot(center-v2, v1-v2)>=0){
			if(dp1*dp1<=r2){
				hits.push_back(isect_t(n, v1, v2, v3, sqrtf(dp1*dp1+dp*dp)));
				continue;
			}
		}
		if(vec3_t::dot(center-v2, v3-v2)>=0 && vec3_t::dot(center-v3, v2-v3)>=0){
			if(dp2*dp2<=r2){
				hits.push_back(isect_t(n, v1, v2, v3, sqrtf(dp2*dp2+dp*dp)));
				continue;
			}
		}
		if(vec3_t::dot(center-v3, v1-v3)>=0 && vec3_t::dot(center-v1, v3-v1)>=0){
			if(dp3*dp3<=r2){
				hits.push_back(isect_t(n, v1, v2, v3, sqrtf(dp3*dp3+dp*dp)));
				continue;
			}
		}
		if((dp=(center-v1).length())<radius){
			hits.push_back(isect_t(n, v1, v2, v3, dp));
		}
		if((dp=(center-v2).length())<radius){
			hits.push_back(isect_t(n, v1, v2, v3, dp));
		}
		if((dp=(center-v3).length())<radius){
			hits.push_back(isect_t(n, v1, v2, v3, dp));
		}
		
	}
	return hit2;
}
void mesh::makeRayCache(){
	if(rayCache)
		return;
	rayCache=new plane_t[count_face*4];
	rayCache2=new aabb_t[count_face];
	
	int n;
	for(n=0;n<count_face;n++){
		vec3_t v[3];
		
		v[0]=vertex[face[(n<<2)]];
		v[1]=vertex[face[(n<<2)+1]];
		v[2]=vertex[face[(n<<2)+2]];
		
		plane_t plane(v[0], v[1], v[2]);
		
		rayCache[n*4+0]=plane;
		rayCache[n*4+1]=plane_t(v[0], v[1], v[0]+plane.n).toward(v[2]);
		rayCache[n*4+2]=plane_t(v[1], v[2], v[1]+plane.n).toward(v[0]);
		rayCache[n*4+3]=plane_t(v[2], v[0], v[2]+plane.n).toward(v[1]);
		
		rayCache2[n]=aabb_t(v[0], v[1], v[2]);
	}
	
	
}
static bool rayTomas(vec3_t vec1, vec3_t vec2, vec3_t v0, vec3_t v1, vec3_t v2, float& dist){
	vec3_t e1, e2;
	vec3_t orig=vec1;
	vec3_t dir=(vec2-vec1).normalize();
	float t, u, v;
	float det, idet;
	vec3_t pvec,tvec, qvec;
	
	e1=v1-v0;
	e2=v2-v0;
	pvec=vec3_t::cross(dir, e2);
	det=vec3_t::dot(e1, pvec);
	
	if(det>.00000001f){
		
		tvec=orig-v0;
		u=vec3_t::dot(tvec, pvec);
		
		if(u<0.f || u>det)
			return false;
		
		qvec=vec3_t::cross(tvec, e1);
		
		v=vec3_t::dot(dir, qvec);
		
		if(v<0.f || u+v>det)
			return false;
		
	}else if(det<-.00000001f){
		
		tvec=orig-v0;
		u=vec3_t::dot(tvec, pvec);
		
		if(u>0.f || u<det)
			return false;
		
		qvec=vec3_t::cross(tvec, e1);
		
		v=vec3_t::dot(dir, qvec);
		
		if(v>0.f || u+v<det)
			return false;
		
	}else{
		return false;
	}
	
	t=vec3_t::dot(e2, qvec);
	
	idet=1.f/det;
	t*=idet;
	
	dist=t;
	//dist=-(plane_t(v0, v1, v2).distance(vec1));
	/*
	
	plane_t plane(v0, v1, v2);
	dist=(plane.distance(vec1));
	dist/=vec3_t::dot(dir, -plane.n);
	if(fabs(dist-t)>.01f)
		printf("too much error: %f %f [%f]\n", dist, t, t*det);
	*/
	if(t>(vec2-vec1).length())
		return false;
	if(t<0.0001f)
		return false;
	/*
	if(dist<0.f)
		return false;
	*/
	
	
	return true;
	
}

vector<isect_t> *mesh::raycast(vec3_t vec1, vec3_t vec2,int ex){
	int n;
	//return vector<isect_t>();
	vector<isect_t> *hit2=new vector<isect_t>();
	vector<isect_t> &hits=*hit2;
	float dist=(vec2-vec1).length();
	aabb_t bound(vec1, vec2);
	makeRayCache();
#pragma omp parallel for
	for(n=0;n<count_face;n++){
		if(n==ex)
			continue;
		vec3_t v[3];
		
		v[0]=vertex[face[(n<<2)]];
		v[1]=vertex[face[(n<<2)+1]];
		v[2]=vertex[face[(n<<2)+2]];
		aabb_t bound2=rayCache2[n];
		if(!(bound && bound2)){
			continue;
		}
		
#if 0
		
		plane_t plane=rayCache[n*4+0]; // (v[0], v[1], v[2]); cached
		vec3_t u=vec2-vec1;
		float top=vec3_t::dot(plane.n, (v[0]-vec1));
		float bottom=vec3_t::dot(plane.n, u);
		if(fabs(bottom)<.00000001f){
			continue; // parallel
		}
		if(top>0.f){
			continue; // reversed
		}
		float t=top/bottom;
		if(t<0.f || t>1.f)
			continue;
		
		vec3_t l;
		l=vec1+u*t;
		
		vec3_t vx; plane_t planex;
		
		//vx=v[0]+plane.n;
		planex=rayCache[n*4+1]; //plane_t(v[0], v[1], vx); cached
		if(planex.distance(l)<0.f)
			continue;
		
		//vx=v[1]+plane.n;
		planex=rayCache[n*4+2]; //plane_t(v[1], v[2], vx); cached
		if(planex.distance(l)<0.f)
			continue;
		
		//vx=v[2]+plane.n;
		planex=rayCache[n*4+3]; //plane_t(v[2], v[0], vx); cached
		if(planex.distance(l)<0.f)
			continue;
		
		dist*=t;
		
#else
	
		if(!rayTomas(vec1, vec2, v[0], v[1], v[2], dist))
			continue;
		
#endif
		
		isect_t i;
		i.face=n;
		i.v1=v[0]; i.v2=v[1]; i.v3=v[2];
		i.dist=dist;
#pragma omp critical
		{
			hits.push_back(i);
		}
		
	}
	return hit2;
}
void Mesh_init(){
#if GL_ARB_shader_objects
	
	if(cap_glsl){
		// load bump shader
		for (auto &p: prg_bump1) {
			p = create_program("res/shaders/bump1.vs", "res/shaders/bump1.fs");
			if(p)
				consoleLog("Mesh_init: compiled program \"bump1\"\n");
			else
				consoleLog("Mesh_init: couldn't compile program \"bump1\"\n");
		}
		
		prg_bump2=create_program("res/shaders/bump2.vs", "res/shaders/bump2.fs");
		if(prg_bump2)
			consoleLog("Mesh_init: compiled program \"bump2\"\n");
		else
			consoleLog("Mesh_init: couldn't compile program \"bump2\"\n");
		
		prg_bump3=create_program("res/shaders/bump2.vs", "res/shaders/bump3.fs");
		if(prg_bump3)
			consoleLog("Mesh_init: compiled program \"bump3\"\n");
		else
			consoleLog("Mesh_init: couldn't compile program \"bump3\"\n");

		attr_tU=glGetAttribLocationARB(prg_bump1[0], "tU");
		attr_tV=glGetAttribLocationARB(prg_bump1[0], "tV");
		attr_gIvt=glGetAttribLocationARB(prg_bump1[0], "gIvt");
		attr_sV=glGetAttribLocationARB(prg_bump1[0], "sV");
	}else{
#endif
		consoleLog("Mesh_init: no programs to compile\n");
#if GL_ARB_shader_objects
	}
#endif
}



