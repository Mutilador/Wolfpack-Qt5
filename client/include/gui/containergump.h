
#if !defined(__CONTAINERGUMP_H__)
#define __CONTAINERGUMP_H__

#include "gui/window.h"
#include "gui/itemimage.h"
#include "game/dynamicitem.h"

class cGumpImage;

/*
	Subclass of cItemImage to display items on container gumps.
*/
class cContainerItemImage : public cItemImage {
Q_OBJECT
public:
	cContainerItemImage(cDynamicItem *item);

	void onMouseEnter();
	void onMouseLeave();
	void onClick(QMouseEvent *e);
	void processDoubleClick(QMouseEvent *e);
	void draw(int xoffset, int yoffset);
	void onMouseDown(QMouseEvent *e);

	uint serial() const;
protected:
	uint serial_; // We reference the entity by serial here
	ushort originalHue;
};

inline uint cContainerItemImage::serial() const {
	return serial_;
}

class cContainerGump : public cWindow {
Q_OBJECT
public:
	cContainerGump(ushort id, ushort hue);
	~cContainerGump();

	cDynamicItem *container() const;
    void setContainer(cDynamicItem *container);
	void refreshContent();
	void flagContentChanged();

	void onClick(QMouseEvent *e);
	void draw(int xoffset, int yoffset);
protected:
	cDynamicItem *container_; // Associated container
	ushort id_; // Background id
	ushort hue_; // Background hue
	cGumpImage *background; // This doesn't get deleted
	bool contentChanged;
};

inline cDynamicItem *cContainerGump::container() const {
	return container_;
}

inline void cContainerGump::flagContentChanged() {
	contentChanged = true;
}

#endif
