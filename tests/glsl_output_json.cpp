#include <string>
#include <vector>
#include <iostream>
#include <time.h>
#include "../src/glsl/glsl_optimizer.h"

#define GL_GLEXT_PROTOTYPES 1

#if __linux__
#define GOT_GFX 0
#else
#define GOT_GFX 1
#endif

#if GOT_GFX

// ---- Windows GL bits
#ifdef _MSC_VER
#define GOT_MORE_THAN_GLSL_120 1
#include <windows.h>
#include <gl/GL.h>
extern "C" {
typedef char GLcharARB;		/* native character */
typedef unsigned int GLhandleARB;	/* shader object handle */
#define GL_VERTEX_SHADER_ARB              0x8B31
#define GL_FRAGMENT_SHADER_ARB            0x8B30
#define GL_OBJECT_COMPILE_STATUS_ARB      0x8B81
typedef void (WINAPI * PFNGLDELETEOBJECTARBPROC) (GLhandleARB obj);
typedef GLhandleARB (WINAPI * PFNGLCREATESHADEROBJECTARBPROC) (GLenum shaderType);
typedef void (WINAPI * PFNGLSHADERSOURCEARBPROC) (GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, const GLint *length);
typedef void (WINAPI * PFNGLCOMPILESHADERARBPROC) (GLhandleARB shaderObj);
typedef void (WINAPI * PFNGLGETINFOLOGARBPROC) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
typedef void (WINAPI * PFNGLGETOBJECTPARAMETERIVARBPROC) (GLhandleARB obj, GLenum pname, GLint *params);
static PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
static PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
static PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
static PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
static PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
static PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
}
#endif // #ifdef _MSC_VER


// ---- Apple GL bits
#ifdef __APPLE__

#define GOT_MORE_THAN_GLSL_120 0
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/CGLTypes.h>
#include <dirent.h>
static CGLContextObj s_GLContext;
static CGLContextObj s_GLContext3;
static bool s_GL3Active = false;

#endif // ifdef __APPLE__


#else // #if GOT_GFX

#define GOT_MORE_THAN_GLSL_120 0
#include <cstdio>
#include <cstring>
#include "dirent.h"
#include "GL/gl.h"
#include "GL/glext.h"

#endif // ! #if GOT_GFX


#ifndef _MSC_VER
#include <unistd.h>
#endif


static bool InitializeOpenGL ()
{
	bool hasGLSL = false;

#if GOT_GFX

#ifdef _MSC_VER
	// setup minimal required GL
	HWND wnd = CreateWindowA(
		"STATIC",
		"GL",
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS |	WS_CLIPCHILDREN,
		0, 0, 16, 16,
		NULL, NULL,
		GetModuleHandle(NULL), NULL );
	HDC dc = GetDC( wnd );

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
		PFD_TYPE_RGBA, 32,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0,
		16, 0,
		0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};

	int fmt = ChoosePixelFormat( dc, &pfd );
	SetPixelFormat( dc, fmt, &pfd );

	HGLRC rc = wglCreateContext( dc );
	wglMakeCurrent( dc, rc );
	
#elif defined(__APPLE__)
	
	CGLPixelFormatAttribute attributes[] = {
		kCGLPFAAccelerated,   // no software rendering
		(CGLPixelFormatAttribute) 0
	};
	CGLPixelFormatAttribute attributes3[] = {
		kCGLPFAAccelerated,   // no software rendering
		kCGLPFAOpenGLProfile, // core profile with the version stated below
		(CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core,
		(CGLPixelFormatAttribute) 0
	};
	GLint num;
	CGLPixelFormatObj pix;
	
	// create legacy context
	CGLChoosePixelFormat(attributes, &pix, &num);
	if (pix == NULL)
		return false;
	CGLCreateContext(pix, NULL, &s_GLContext);
	if (s_GLContext == NULL)
		return false;
	CGLDestroyPixelFormat(pix);
	CGLSetCurrentContext(s_GLContext);
	
	// create core 3.2 context
	CGLChoosePixelFormat(attributes3, &pix, &num);
	if (pix == NULL)
		return false;
	CGLCreateContext(pix, NULL, &s_GLContext3);
	if (s_GLContext3 == NULL)
		return false;
	CGLDestroyPixelFormat(pix);

#endif

	// check if we have GLSL
	const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
	hasGLSL = extensions != NULL && strstr(extensions, "GL_ARB_shader_objects") && strstr(extensions, "GL_ARB_vertex_shader") && strstr(extensions, "GL_ARB_fragment_shader");
	
	#if defined(__APPLE__)
	// using core profile; always has GLSL
	hasGLSL = true;
	#endif
	
	
#ifdef _MSC_VER
	if (hasGLSL)
	{
		glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC)wglGetProcAddress("glDeleteObjectARB");
		glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)wglGetProcAddress("glCreateShaderObjectARB");
		glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)wglGetProcAddress("glShaderSourceARB");
		glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)wglGetProcAddress("glCompileShaderARB");
		glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)wglGetProcAddress("glGetInfoLogARB");
		glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)wglGetProcAddress("glGetObjectParameterivARB");
	}
#endif

#endif
	return hasGLSL;
}

static void CleanupGL()
{
#if GOT_GFX
	#ifdef __APPLE__
	CGLSetCurrentContext(NULL);
	if (s_GLContext)
		CGLDestroyContext(s_GLContext);
	if (s_GLContext3)
		CGLDestroyContext(s_GLContext3);
	#endif // #ifdef __APPLE__
#endif // #if GOT_GFX
}

static void replace_string (std::string& target, const std::string& search, const std::string& replace, size_t startPos)
{
	if (search.empty())
		return;
	
	std::string::size_type p = startPos;
	while ((p = target.find (search, p)) != std::string::npos)
	{
		target.replace (p, search.size (), replace);
		p += replace.size ();
	}
}

static bool CheckGLSL (bool vertex, bool gles, const std::string& testName, const char* prefix, const std::string& source)
{
	#if !GOT_GFX
	return true; // just assume it's ok
	#endif

	#if !GOT_MORE_THAN_GLSL_120
	if (source.find("#version 140") != std::string::npos)
		return true;
	#endif
	
	bool need3 = false;
#	ifdef __APPLE__
	// Mac core context does not accept any older shader versions, so need to switch to
	// either legacy context or core one.
	need3 =
		(source.find("#version 150") != std::string::npos) ||
		(source.find("#version 300") != std::string::npos);
	if (need3)
	{
		if (!s_GL3Active)
			CGLSetCurrentContext(s_GLContext3);
		s_GL3Active = true;
	}
	else
	{
		if (s_GL3Active)
			CGLSetCurrentContext(s_GLContext);
		s_GL3Active = false;
	}
#	endif // ifdef __APPLE__
	
	
	std::string src;
	if (gles)
	{
		src += "#define lowp\n";
		src += "#define mediump\n";
		src += "#define highp\n";
		src += "#define texture2DLodEXT texture2DLod\n";
		src += "#define texture2DProjLodEXT texture2DProjLod\n";
		src += "#define texture2DGradEXT texture2DGradARB\n";
		src += "#define textureCubeGradEXT textureCubeGradARB\n";
		src += "#define gl_FragDepthEXT gl_FragDepth\n";
		if (!need3)
		{
			src += "#define gl_LastFragData _glesLastFragData\n";
			src += "varying lowp vec4 _glesLastFragData[4];\n";
		}
		src += "float shadow2DEXT (sampler2DShadow s, vec3 p) { return shadow2D(s,p).r; }\n";
		src += "float shadow2DProjEXT (sampler2DShadow s, vec4 p) { return shadow2DProj(s,p).r; }\n";
	}
	src += source;
	if (gles)
	{
		replace_string (src, "GL_EXT_shader_texture_lod", "GL_ARB_shader_texture_lod", 0);
		replace_string (src, "#extension GL_OES_standard_derivatives : require", "", 0);
		replace_string (src, "#extension GL_EXT_shadow_samplers : require", "", 0);
		replace_string (src, "#extension GL_EXT_frag_depth : require", "", 0);
		replace_string (src, "#extension GL_OES_standard_derivatives : enable", "", 0);
		replace_string (src, "#extension GL_EXT_shadow_samplers : enable", "", 0);
		replace_string (src, "#extension GL_EXT_frag_depth : enable", "", 0);
		replace_string (src, "#extension GL_EXT_draw_buffers : enable", "", 0);
		replace_string (src, "#extension GL_EXT_draw_buffers : require", "", 0);
		replace_string (src, "precision ", "// precision ", 0);
		replace_string (src, "#version 300 es", "", 0);
	}
	replace_string (src, "#extension GL_EXT_shader_framebuffer_fetch : require", "", 0);
	replace_string (src, "#extension GL_EXT_shader_framebuffer_fetch : enable", "", 0);
	if (gles && need3)
	{
		src = "#version 330\n" + src;
	}
	const char* sourcePtr = src.c_str();

	
	GLhandleARB shader = glCreateShaderObjectARB (vertex ? GL_VERTEX_SHADER_ARB : GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB (shader, 1, &sourcePtr, NULL);
	glCompileShaderARB (shader);
	GLint status;
	glGetObjectParameterivARB (shader, GL_OBJECT_COMPILE_STATUS_ARB, &status);
	bool res = true;
	if (status == 0)
	{
		char log[4096];
		log[0] = 0;
		GLsizei logLength;
		glGetInfoLogARB (shader, sizeof(log), &logLength, log);
		printf ("\n  %s: real glsl compiler error on %s:\n%s\n", testName.c_str(), prefix, log);
		res = false;
	}
	glDeleteObjectARB (shader);
	return res;
}


static bool ReadStringFromFile (const char* pathName, std::string& output)
{
	FILE* file = fopen( pathName, "rb" );
	if (file == NULL)
		return false;
	fseek(file, 0, SEEK_END);
	int length = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (length < 0)
	{
		fclose( file );
		return false;
	}
	output.resize(length);
	int readLength = fread(&*output.begin(), 1, length, file);
	fclose(file);
	if (readLength != length)
	{
		output.clear();
		return false;
	}

	replace_string(output, "\r\n", "\n", 0);
	return true;
}

bool EndsWith (const std::string& str, const std::string& sub)
{
	return (str.size() >= sub.size()) && (strncmp (str.c_str()+str.size()-sub.size(), sub.c_str(), sub.size())==0);
}

typedef std::vector<std::string> StringVector;

static StringVector GetFiles (const std::string& folder, const std::string& endsWith)
{
	StringVector res;

	#ifdef _MSC_VER
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind = FindFirstFileA ((folder+"/*"+endsWith).c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return res;

	do {
		res.push_back (FindFileData.cFileName);
	} while (FindNextFileA (hFind, &FindFileData));

	FindClose (hFind);
	
	#else
	
	DIR *dirp;
	struct dirent *dp;

	if ((dirp = opendir(folder.c_str())) == NULL)
		return res;

	while ( (dp = readdir(dirp)) )
	{
		std::string fname = dp->d_name;
		if (fname == "." || fname == "..")
			continue;
		if (!EndsWith (fname, endsWith))
			continue;
		res.push_back (fname);
	}
	closedir(dirp);
	
	#endif

	return res;
}

static void DeleteFile (const std::string& path)
{
	#ifdef _MSC_VER
	DeleteFileA (path.c_str());
	#else
	unlink (path.c_str());
	#endif
}

static void MassageVertexForGLES (std::string& s)
{
	std::string pre;
	std::string version = "#version 300 es\n";
	size_t insertPoint = s.find(version);
	if (insertPoint != std::string::npos)
	{
		insertPoint += version.size();
		pre += "#define gl_Vertex _glesVertex\nin highp vec4 _glesVertex;\n";
		pre += "#define gl_Normal _glesNormal\nin mediump vec3 _glesNormal;\n";
		pre += "#define gl_MultiTexCoord0 _glesMultiTexCoord0\nin highp vec4 _glesMultiTexCoord0;\n";
		pre += "#define gl_MultiTexCoord1 _glesMultiTexCoord1\nin highp vec4 _glesMultiTexCoord1;\n";
		pre += "#define gl_Color _glesColor\nin lowp vec4 _glesColor;\n";
	}
	else
	{
		insertPoint = 0;
		pre += "#define gl_Vertex _glesVertex\nattribute highp vec4 _glesVertex;\n";
		pre += "#define gl_Normal _glesNormal\nattribute mediump vec3 _glesNormal;\n";
		pre += "#define gl_MultiTexCoord0 _glesMultiTexCoord0\nattribute highp vec4 _glesMultiTexCoord0;\n";
		pre += "#define gl_MultiTexCoord1 _glesMultiTexCoord1\nattribute highp vec4 _glesMultiTexCoord1;\n";
		pre += "#define gl_Color _glesColor\nattribute lowp vec4 _glesColor;\n";
	}
	
	s.insert (insertPoint, pre);
}

static void MassageFragmentForGLES (std::string& s)
{
	std::string pre;
	s = pre + s;
}

static bool TestFile (glslopt_ctx* ctx, bool vertex,
	const std::string& testName,
	const std::string& inputPath,
	const std::string& hirPath,
	const std::string& outputPath,
	const std::string& outPath,
	bool gles,
	bool doCheckGLSL)
{
	std::string input;
	if (!ReadStringFromFile (inputPath.c_str(), input))
	{
		printf ("\n  %s: failed to read input file\n", testName.c_str());
		return false;
	}
	if (doCheckGLSL)
	{
		if (!CheckGLSL (vertex, gles, testName, "input", input.c_str()))
			return false;
	}

	if (gles)
	{
		if (vertex)
			MassageVertexForGLES (input);
		else
			MassageFragmentForGLES (input);
	}

	bool res = true;

	glslopt_shader_type type = vertex ? kGlslOptShaderVertex : kGlslOptShaderFragment;
	glslopt_shader* shader = glslopt_optimize (ctx, type, input.c_str(), 0);

	bool optimizeOk = glslopt_get_status(shader);
	if (optimizeOk)
	{
		std::string textHir = glslopt_get_raw_output (shader);
		std::string textOpt = glslopt_get_output (shader);
		std::string textJson = glslopt_get_json_output (shader);

		char buffer[200];
		int statsAlu, statsTex, statsFlow;
		glslopt_shader_get_stats (shader, &statsAlu, &statsTex, &statsFlow);
		int inputCount = glslopt_shader_get_input_count (shader);
		sprintf(buffer, "\n// inputs: %i, stats: %i alu %i tex %i flow\n", inputCount, statsAlu, statsTex, statsFlow);
		textOpt += buffer;


		std::string jsonPath = outPath + "/" + testName + ".json";
		{
			// write output
			FILE* f = fopen (jsonPath.c_str(), "wb");
			if (!f)
			{
				printf ("\n  %s: can't write to JSON file!\n", testName.c_str());
			}
			else
			{
				fwrite (textJson.c_str(), 1, textJson.size(), f);
				fclose (f);
			}
			res = false;
		}

#if 0
		// write output
		{
			FILE* f = fopen (hirPath.c_str(), "wb");
			if (!f)
			{
				printf ("\n  %s: can't write to IR file!\n", testName.c_str());
			}
			else
			{
				fwrite (textHir.c_str(), 1, textHir.size(), f);
				fclose (f);
			}
		}

		// write output
		{
			FILE* f = fopen (outputPath.c_str(), "wb");
			if (!f)
			{
				printf ("\n  %s: can't write to optimized file!\n", testName.c_str());
			}
			else
			{
				fwrite (textOpt.c_str(), 1, textOpt.size(), f);
				fclose (f);
			}
		}

		if (res && doCheckGLSL && !CheckGLSL (vertex, gles, testName, "raw", textHir.c_str()))
			res = false;
		if (res && doCheckGLSL && !CheckGLSL (vertex, gles, testName, "optimized", textOpt.c_str()))
			res = false;
#endif
	}
	else
	{
		printf ("\n  %s: optimize error: %s\n", testName.c_str(), glslopt_get_log(shader));
		res = false;
	}

	glslopt_shader_delete (shader);

	return res;
}


// http://stackoverflow.com/questions/1494399/how-do-i-search-find-and-replace-in-a-standard-string
void myReplace(std::string& str, const std::string& oldStr, const std::string& newStr)
{
	size_t pos = 0;
	while((pos = str.find(oldStr, pos)) != std::string::npos)
	{
		str.replace(pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
}

int main (int argc, const char** argv)
{
	if (argc < 2)
	{
		printf ("USAGE: glsloptimizer testfolder\n");
		return 1;
	}

	bool hasOpenGL = InitializeOpenGL ();
	glslopt_ctx* ctx[3] = {
		glslopt_initialize(kGlslTargetOpenGLES20),
		glslopt_initialize(kGlslTargetOpenGLES30),
		glslopt_initialize(kGlslTargetOpenGL),
	};

	std::string baseFolder = argv[1];


	// write file manifest
	std::string manifest = baseFolder + "/manifest.html";
	FILE* fman = fopen (manifest.c_str(), "wb");
	if (!fman)
	{
		printf ("\n  %s: can't write to optimized file!\n", manifest.c_str());
	}
	// Add html header to file
	std::string htmlHeaderFile = baseFolder + "/shader_trees_header.html";
	std::string htmlHeader;
	ReadStringFromFile (htmlHeaderFile.c_str(), htmlHeader);
	fwrite (htmlHeader.c_str(), 1, htmlHeader.size(), fman);

	
	std::string testFolder = baseFolder + "/shaders";
	std::string outFolder = baseFolder + "/json";
	const char* suffix = "-in.txt";
	StringVector inputFiles = GetFiles (testFolder, suffix);

	size_t errors = 0;
	size_t n = inputFiles.size();
	std::cout << "Compiling " << n << " sample files..." << std::endl;
	for (size_t i = 0; i < n; ++i)
	{
		std::string inname = inputFiles[i];
		std::cout << "  - " << inname << "...";
		//if (inname != "ast-in.txt")
		//	continue;
		std::string hirname = inname.substr (0,inname.size()-strlen(suffix)) + "-hir.txt";
		std::string outname = inname.substr (0,inname.size()-strlen(suffix)) + "-out.txt";
		bool ok = TestFile (
			ctx[0],
			false,
			inname,
			testFolder + "/" + inname,
			testFolder + "/" + hirname,
			testFolder + "/" + outname,
			outFolder,
			false,
			hasOpenGL);
		if (!ok)
		{
			++errors;
		}

		std::string jsonPath = inname + ".json";
		std::string ahref = "<a href=\"#\" onclick=\"LoadShaderFromJson('json/{0}');return false;\">{0}</a><br>\n";
		std::string jline = ahref;
		myReplace(jline, "{0}", jsonPath);
		fwrite (jline.c_str(), 1, jline.size(), fman);
		std::cout << "done." << std::endl;
	}

	// Add html footer to file
	std::string htmlFooterFile = baseFolder + "/shader_trees_footer.html";
	std::string htmlFooter;
	ReadStringFromFile (htmlFooterFile.c_str(), htmlFooter);
	fwrite (htmlFooter.c_str(), 1, htmlFooter.size(), fman);

	fclose (fman);


	for (int i = 0; i < 2; ++i)
		glslopt_cleanup (ctx[i]);
	CleanupGL();
	
#ifdef _MSC_VER
	// Prevent window from closing on finish
	getchar();
#endif

	return errors ? 1 : 0;
}