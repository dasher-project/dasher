#ifndef __dasher_buffer_set_h__
#define __dasher_buffer_set_h__

#include <glib-object.h>

#define IDASHER_BUFFER_SET_TYPE (idasher_buffer_set_get_type())
#define IDASHER_BUFFER_SET(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IDASHER_BUFFER_SET_TYPE, IDasherBufferSet))
#define IS_IDASHER_BUFFER_SET(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IDASHER_BUFFER_SET_TYPE))
#define IDASHER_BUFFER_SET_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), IDASHER_BUFFER_SET_TYPE, IDasherBufferSetInterface))

enum {
  CONTEXT_CHANGED,
  NUM_SIGNALS
};

// Basic cursor movement commands

enum {
  EDIT_FORWARDS, 
  EDIT_BACKWARDS
};
  
enum {
  EDIT_CHAR,
  EDIT_WORD,
  EDIT_LINE,
  EDIT_FILE
};

extern guint idasher_buffer_set_signals[];

typedef struct _IDasherBufferSet IDasherBufferSet;
typedef struct _IDasherBufferSetInterface IDasherBufferSetInterface;

struct _IDasherBufferSetInterface {
  GTypeInterface parent;

  void (*insert_text)(IDasherBufferSet *pSelf, const gchar *szText);
  void (*delete_text)(IDasherBufferSet *pSelf, gint iLength);
  gchar *(*get_context)(IDasherBufferSet *pSelf, gint iMaxLength);
  void (*edit_move)(IDasherBufferSet *pSelf, gint iDirection, gint iDist);
  void (*edit_delete)(IDasherBufferSet *pSelf, gint iDirection, gint iDist);
};

GType idasher_buffer_set_get_type();

void idasher_buffer_set_insert(IDasherBufferSet *pSelf, const gchar *szText);
void idasher_buffer_set_delete(IDasherBufferSet *pSelf, gint iLength);
gchar *idasher_buffer_set_get_context(IDasherBufferSet *pSelf, gint iMaxLength);
void idasher_buffer_set_edit_move(IDasherBufferSet *pSelf, gint iDirection, gint iDist);
void idasher_buffer_set_edit_delete(IDasherBufferSet *pSelf, gint iDirection, gint iDist);

#endif