#ifndef _GLFRAGMENT_H
#define _GLFRAGMENT_H

#define MAX_FRAGMENT_FUNCTIONS 16

#define COMP_FETCH_TARGET_2D   0
#define COMP_FETCH_TARGET_RECT 1
#define COMP_FETCH_TARGET_NUM  2

struct GLWindowPaintAttrib;
class GLScreen;

namespace GLFragment {

    class Storage;

    typedef unsigned int FunctionId;

    class PrivateFunctionData;
    class PrivateAttrib;

    class FunctionData {
	public:
	    FunctionData ();
	    ~FunctionData ();

	    bool status ();

	    void addTempHeaderOp (const char *name);

	    void addParamHeaderOp (const char *name);

	    void addAttribHeaderOp (const char *name);


	    void addFetchOp (const char *dst, const char *offset, int target);

	    void addColorOp (const char *dst, const char *src);

	    void addDataOp (const char *str, ...);

	    void addBlendOp (const char *str, ...);

	    FunctionId createFragmentFunction (const char *name);

	private:
	    PrivateFunctionData *priv;
    };

    class Attrib {
	public:
	    Attrib (const GLWindowPaintAttrib &paint);
	    Attrib (const Attrib&);
	    ~Attrib ();

	    unsigned int allocTextureUnits (unsigned int nTexture);

	    unsigned int allocParameters (unsigned int nParam);

	    void addFunction (FunctionId function);

	    bool enable (bool *blending);
	    void disable ();

	    unsigned short getSaturation ();
	    unsigned short getBrightness ();
	    unsigned short getOpacity ();

	    void setSaturation (unsigned short);
	    void setBrightness (unsigned short);
	    void setOpacity (unsigned short);

	    bool hasFunctions ();

	private:
	    PrivateAttrib *priv;
    };

    void destroyFragmentFunction (FunctionId id);

    FunctionId getSaturateFragmentFunction (GLTexture *texture,
					    int       param);
};



#endif
