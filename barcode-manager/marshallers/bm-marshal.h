
#ifndef ___bm_marshal_MARSHAL_H__
#define ___bm_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:OBJECT (bm-marshal.list:1) */
#define _bm_marshal_VOID__OBJECT	g_cclosure_marshal_VOID__OBJECT

/* VOID:OBJECT,STRING (bm-marshal.list:2) */
extern void _bm_marshal_VOID__OBJECT_STRING (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data);

/* VOID:OBJECT,STRING,UINT (bm-marshal.list:3) */
extern void _bm_marshal_VOID__OBJECT_STRING_UINT (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);

/* VOID:OBJECT,UINT (bm-marshal.list:4) */
extern void _bm_marshal_VOID__OBJECT_UINT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* VOID:OBJECT,POINTER (bm-marshal.list:5) */
extern void _bm_marshal_VOID__OBJECT_POINTER (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* VOID:OBJECT,POINTER,UINT (bm-marshal.list:6) */
extern void _bm_marshal_VOID__OBJECT_POINTER_UINT (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* VOID:POINTER (bm-marshal.list:7) */
#define _bm_marshal_VOID__POINTER	g_cclosure_marshal_VOID__POINTER

/* VOID:STRING,STRING,STRING (bm-marshal.list:8) */
extern void _bm_marshal_VOID__STRING_STRING_STRING (GClosure     *closure,
                                                    GValue       *return_value,
                                                    guint         n_param_values,
                                                    const GValue *param_values,
                                                    gpointer      invocation_hint,
                                                    gpointer      marshal_data);

/* VOID:UINT,UINT (bm-marshal.list:9) */
extern void _bm_marshal_VOID__UINT_UINT (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);

/* VOID:UINT,UINT,UINT (bm-marshal.list:10) */
extern void _bm_marshal_VOID__UINT_UINT_UINT (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* VOID:STRING,STRING (bm-marshal.list:11) */
extern void _bm_marshal_VOID__STRING_STRING (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data);

/* VOID:STRING,UCHAR (bm-marshal.list:12) */
extern void _bm_marshal_VOID__STRING_UCHAR (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);

/* VOID:STRING,OBJECT (bm-marshal.list:13) */
extern void _bm_marshal_VOID__STRING_OBJECT (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data);

/* VOID:POINTER,POINTER (bm-marshal.list:14) */
extern void _bm_marshal_VOID__POINTER_POINTER (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* VOID:STRING,STRING,STRING,UINT (bm-marshal.list:15) */
extern void _bm_marshal_VOID__STRING_STRING_STRING_UINT (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

/* VOID:OBJECT,UINT,UINT (bm-marshal.list:16) */
extern void _bm_marshal_VOID__OBJECT_UINT_UINT (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* VOID:STRING,INT (bm-marshal.list:17) */
extern void _bm_marshal_VOID__STRING_INT (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

/* VOID:INT,UINT (bm-marshal.list:18) */
extern void _bm_marshal_VOID__INT_UINT (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);

/* VOID:INT,UINT,BOOLEAN (bm-marshal.list:19) */
extern void _bm_marshal_VOID__INT_UINT_BOOLEAN (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* VOID:OBJECT,OBJECT,ENUM (bm-marshal.list:20) */
extern void _bm_marshal_VOID__OBJECT_OBJECT_ENUM (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);

/* VOID:POINTER,STRING (bm-marshal.list:21) */
extern void _bm_marshal_VOID__POINTER_STRING (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* VOID:STRING,BOXED (bm-marshal.list:22) */
extern void _bm_marshal_VOID__STRING_BOXED (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);

/* BOOLEAN:POINTER,STRING,BOOLEAN,UINT,STRING,STRING (bm-marshal.list:23) */
extern void _bm_marshal_BOOLEAN__POINTER_STRING_BOOLEAN_UINT_STRING_STRING (GClosure     *closure,
                                                                            GValue       *return_value,
                                                                            guint         n_param_values,
                                                                            const GValue *param_values,
                                                                            gpointer      invocation_hint,
                                                                            gpointer      marshal_data);

/* VOID:STRING,BOOLEAN,UINT,STRING,STRING (bm-marshal.list:24) */
extern void _bm_marshal_VOID__STRING_BOOLEAN_UINT_STRING_STRING (GClosure     *closure,
                                                                 GValue       *return_value,
                                                                 guint         n_param_values,
                                                                 const GValue *param_values,
                                                                 gpointer      invocation_hint,
                                                                 gpointer      marshal_data);

/* BOOLEAN:VOID (bm-marshal.list:25) */
extern void _bm_marshal_BOOLEAN__VOID (GClosure     *closure,
                                       GValue       *return_value,
                                       guint         n_param_values,
                                       const GValue *param_values,
                                       gpointer      invocation_hint,
                                       gpointer      marshal_data);

/* VOID:STRING,BOOLEAN (bm-marshal.list:26) */
extern void _bm_marshal_VOID__STRING_BOOLEAN (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* VOID:STRING,OBJECT,POINTER (bm-marshal.list:27) */
extern void _bm_marshal_VOID__STRING_OBJECT_POINTER (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);

/* VOID:BOOLEAN,UINT (bm-marshal.list:28) */
extern void _bm_marshal_VOID__BOOLEAN_UINT (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);

G_END_DECLS

#endif /* ___bm_marshal_MARSHAL_H__ */

