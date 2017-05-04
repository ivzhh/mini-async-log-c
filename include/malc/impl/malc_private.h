#ifndef __MALC_PRIVATE_H__
#define __MALC_PRIVATE_H__

#if !defined (__MALC_H__)
  #error "This file should be included through malc.h"
#endif

#include <bl/base/integer.h>
#include <bl/base/integer_manipulation.h>
#include <bl/base/integer_math.h>
#include <bl/base/error.h>
#include <bl/base/preprocessor.h>
#include <bl/base/utility.h>

#include <malc/libexport.h>

/*----------------------------------------------------------------------------*/
#if defined (MALC_NO_BUILTIN_COMPRESSION) && defined (MALC_NO_PTR_COMPRESSION)
  #define MALC_NO_COMPRESSION
#endif
#if defined (MALC_NO_COMPRESSION) &&\
  (!defined (MALC_NO_BUILTIN_COMPRESSION) || !defined (MALC_NO_PTR_COMPRESSION))
  #error "contradictory compression setup"
#endif
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
  extern "C" {
#endif
/*----------------------------------------------------------------------------*/
typedef enum malc_encodings {
  malc_end          = 0,
  malc_type_i8      = 'a',
  malc_type_u8      = 'b',
  malc_type_i16     = 'c',
  malc_type_u16     = 'd',
  malc_type_i32     = 'e',
  malc_type_u32     = 'f',
  malc_type_float   = 'g',
  malc_type_i64     = 'h',
  malc_type_u64     = 'i',
  malc_type_double  = 'j',
  malc_type_ptr     = 'k',
  malc_type_lit     = 'l',
  malc_type_strcp   = 'm',
  malc_type_memcp   = 'n',
  malc_type_error   = 'o',
  malc_sev_debug    = '3',
  malc_sev_trace    = '4',
  malc_sev_note     = '5',
  malc_sev_warning  = '6',
  malc_sev_error    = '7',
  malc_sev_critical = '8',
  malc_sev_off      = '9',
}
malc_type_ids;
/*----------------------------------------------------------------------------*/
typedef struct malc_lit {
  char const* lit;
}
malc_lit;
/*----------------------------------------------------------------------------*/
typedef struct malc_memcp {
  u8 const* mem;
  u16       size;
}
malc_memcp;
/*----------------------------------------------------------------------------*/
typedef struct malc_strcp {
  char const* str;
  u16         len;
}
malc_strcp;
/*----------------------------------------------------------------------------*/
typedef struct malc_const_entry {
  char const* format;
  char const* info;
  u16         compressed_count;
}
malc_const_entry;
/*----------------------------------------------------------------------------*/
typedef struct malc_compressed_32 {
  u32 v;
  u32 format_nibble; /*1 bit sign + 3 bit size (0-7)*/
}
malc_compressed_32;
/*----------------------------------------------------------------------------*/
typedef struct malc_compressed_64 {
  u64   v;
  uword format_nibble; /*1 bit sign + 3 bit size (0-7)*/
}
malc_compressed_64;
/*----------------------------------------------------------------------------*/
static inline malc_compressed_32 malc_get_compressed_u32 (u32 v)
{
  malc_compressed_32 r;
  r.v              = v;
  r.format_nibble  = v ? log2_floor_unsafe_u32 (v) / 8 : 0;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline malc_compressed_32 malc_get_compressed_i32 (i32 v)
{
  malc_compressed_32 r = malc_get_compressed_u32 ((u32) (v < 0 ? ~v : v));
  r.format_nibble     |= (v < 0) << 3;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline malc_compressed_64 malc_get_compressed_u64 (u64 v)
{
  malc_compressed_64 r;
  r.v              = v;
  r.format_nibble  = v ? log2_floor_unsafe_u64 (v) / 8 : 0;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline malc_compressed_64 malc_get_compressed_i64 (i64 v)
{
  malc_compressed_64 r = malc_get_compressed_u64 ((u64) (v < 0 ? ~v : v));
  r.format_nibble     |= (v < 0) << 3;
  return r;
}
/*----------------------------------------------------------------------------*/
#if BL_WORDSIZE == 64
  typedef malc_compressed_64 malc_compressed_ptr;
  static inline malc_compressed_ptr malc_get_compressed_ptr (void* v)
  {
    return malc_get_compressed_u64 ((u64) v);
  }
#elif BL_WORDSIZE == 32
  typedef malc_compressed_32 malc_compressed_ptr;
  static inline malc_compressed_ptr malc_get_compressed_ptr (void* v)
  {
    return malc_get_compressed_u32 ((u32) v);
  }
#else
  #error "Unsupported word size or bad compiler detection"
#endif
/*----------------------------------------------------------------------------*/
struct malc;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_log(
  struct malc* l, malc_const_entry const* e, uword size, ...
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT uword malc_get_min_severity (struct malc const* l);
/*----------------------------------------------------------------------------*/
#ifdef MALC_NO_PTR_COMPRESSION
  #define malc_ptr_is_compressed(x) 0
#else
  #define malc_ptr_is_compressed(x)\
    ((int) (malc_get_type_code ((x)) == malc_type_ptr || \
            malc_get_type_code ((x)) == malc_type_lit))
#endif

#ifdef MALC_NO_BUILTIN_COMPRESSION
  #define malc_builtin_is_compressed(x) 0
#else
  #define malc_builtin_is_compressed(x)\
    ((int) (malc_get_type_code ((x)) == malc_type_i32 || \
            malc_get_type_code ((x)) == malc_type_u32 || \
            malc_get_type_code ((x)) == malc_type_i64 || \
            malc_get_type_code ((x)) == malc_type_u64))
#endif

#define malc_is_compressed(x)\
  (malc_builtin_is_compressed (x) || malc_ptr_is_compressed (x))
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus

#define malc_tgen_cv_cases(type, expression)\
  type:                (expression),\
  const type:          (expression),\
  volatile type:       (expression),\
  const volatile type: (expression)

#define malc_get_type_code(expression)\
  _Generic ((expression),\
    malc_tgen_cv_cases (float,       (char) malc_type_float),\
    malc_tgen_cv_cases (double,      (char) malc_type_double),\
    malc_tgen_cv_cases (i8,          (char) malc_type_i8),\
    malc_tgen_cv_cases (u8,          (char) malc_type_u8),\
    malc_tgen_cv_cases (i16,         (char) malc_type_i16),\
    malc_tgen_cv_cases (u16,         (char) malc_type_u16),\
    malc_tgen_cv_cases (i32,         (char) malc_type_i32),\
    malc_tgen_cv_cases (u32,         (char) malc_type_u32),\
    malc_tgen_cv_cases (i64,         (char) malc_type_i64),\
    malc_tgen_cv_cases (u64,         (char) malc_type_u64),\
    malc_tgen_cv_cases (void*,       (char) malc_type_ptr),\
    malc_tgen_cv_cases (void* const, (char) malc_type_ptr),\
    malc_lit:                        (char) malc_type_lit,\
    malc_strcp:                      (char) malc_type_strcp,\
    malc_memcp:                      (char) malc_type_memcp,\
    default:                         (char) malc_type_error\
    )

static inline uword malc_size_float  (float v)       { return sizeof (v); }
static inline uword malc_size_double (double v)      { return sizeof (v); }
static inline uword malc_size_i8     (i8 v)          { return sizeof (v); }
static inline uword malc_size_u8     (u8 v)          { return sizeof (v); }
static inline uword malc_size_i16    (i16 v)         { return sizeof (v); }
static inline uword malc_size_u16    (u16 v)         { return sizeof (v); }
static inline uword malc_size_i32    (i32 v)         { return sizeof (v); }
static inline uword malc_size_u32    (u32 v)         { return sizeof (v); }
static inline uword malc_size_i64    (i64 v)         { return sizeof (v); }
static inline uword malc_size_u64    (u64 v)         { return sizeof (v); }
static inline uword malc_size_ptr    (void* v)       { return sizeof (v); }
static inline uword malc_size_ptrc   (void* const v) { return sizeof (v); }
static inline uword malc_size_lit    (malc_lit v)    { return sizeof (v); }

static inline uword malc_size_strcp (malc_strcp v)
{
  return sizeof_member (malc_strcp, len) + v.len;
}
static inline uword malc_size_memcp (malc_memcp v)
{
  return sizeof_member (malc_memcp, size) + v.size;
}
static inline uword malc_size_comp32 (malc_compressed_32 v)
{
  return (v.format_nibble & (u_bit (3) - 1)) + 1;
}
static inline uword malc_size_comp64 (malc_compressed_64 v)
{
  return (v.format_nibble & (u_bit (3) - 1)) + 1;
}

#define malc_type_size(expression)\
  _Generic ((expression),\
    malc_tgen_cv_cases (float,       malc_size_float),\
    malc_tgen_cv_cases (double,      malc_size_double),\
    malc_tgen_cv_cases (i8,          malc_size_i8),\
    malc_tgen_cv_cases (u8,          malc_size_u8),\
    malc_tgen_cv_cases (i16,         malc_size_i16),\
    malc_tgen_cv_cases (u16,         malc_size_u16),\
    malc_tgen_cv_cases (i32,         malc_size_i32),\
    malc_tgen_cv_cases (u32,         malc_size_u32),\
    malc_tgen_cv_cases (i64,         malc_size_i64),\
    malc_tgen_cv_cases (u64,         malc_size_u64),\
    malc_tgen_cv_cases (void*,       malc_size_ptr),\
    malc_tgen_cv_cases (void* const, malc_size_ptrc),\
    malc_compressed_32:              malc_size_comp32,\
    malc_compressed_64:              malc_size_comp64,\
    malc_lit:                        malc_size_lit,\
    malc_strcp:                      malc_size_strcp,\
    malc_memcp:                      malc_size_memcp,\
    default:                         malc_size_ptr\
    )\
  (expression)

static inline float       malc_transform_float     (float v)       { return v; }
static inline double      malc_transform_double    (double v)      { return v; }
static inline i8          malc_transform_i8        (i8 v)          { return v; }
static inline u8          malc_transform_u8        (u8 v)          { return v; }
static inline i16         malc_transform_i16       (i16 v)         { return v; }
static inline u16         malc_transform_u16       (u16 v)         { return v; }
#ifdef MALC_NO_BUILTIN_COMPRESSION
  static inline i32       malc_transform_i32       (i32 v)         { return v; }
  static inline u32       malc_transform_u32       (u32 v)         { return v; }
  static inline i64       malc_transform_i64       (i64 v)         { return v; }
  static inline u64       malc_transform_u64       (u64 v)         { return v; }
#else
  static inline malc_compressed_32 malc_transform_i32 (i32 v)
  {
    return malc_get_compressed_i32 (v);
  }
  static inline malc_compressed_32 malc_transform_u32 (u32 v)
  {
    return malc_get_compressed_u32 (v);
  }
  static inline malc_compressed_64 malc_transform_i64 (i64 v)
  {
    return malc_get_compressed_i64 (v);
  }
  static inline malc_compressed_64 malc_transform_u64 (u64 v)
  {
    return malc_get_compressed_u64 (v);
  }
#endif
#ifdef MALC_NO_PTR_COMPRESSION
  static inline void*       malc_transform_ptr  (void* v)       { return v; }
  static inline void* const malc_transform_ptrc (void* const v) { return v; }
  static inline malc_lit    malc_transform_lit  (malc_lit v)    { return v; }
#else
  static inline malc_compressed_ptr malc_transform_ptr (void* v)
  {
    return malc_get_compressed_ptr (v);
  }
  static inline malc_compressed_ptr malc_transform_ptrc (void* const v)
  {
    return malc_get_compressed_ptr ((void*) v);
  }
  static inline malc_compressed_ptr malc_transform_lit  (malc_lit v)
  {
    return malc_get_compressed_ptr ((void*) v.lit);
  }
#endif
static inline malc_strcp  malc_transform_str  (malc_strcp v)  { return v; }
static inline malc_memcp  malc_transform_mem  (malc_memcp v)  { return v; }

#define malc_type_transform(expression)\
  _Generic ((expression),\
    malc_tgen_cv_cases (float,       malc_transform_float),\
    malc_tgen_cv_cases (double,      malc_transform_double),\
    malc_tgen_cv_cases (i8,          malc_transform_i8),\
    malc_tgen_cv_cases (u8,          malc_transform_u8),\
    malc_tgen_cv_cases (i16,         malc_transform_i16),\
    malc_tgen_cv_cases (u16,         malc_transform_u16),\
    malc_tgen_cv_cases (i32,         malc_transform_i32),\
    malc_tgen_cv_cases (u32,         malc_transform_u32),\
    malc_tgen_cv_cases (i64,         malc_transform_i64),\
    malc_tgen_cv_cases (u64,         malc_transform_u64),\
    malc_tgen_cv_cases (void*,       malc_transform_ptr),\
    malc_tgen_cv_cases (void* const, malc_transform_ptrc),\
    malc_lit:                        malc_transform_lit,\
    malc_strcp:                      malc_transform_str,\
    malc_memcp:                      malc_transform_mem,\
    default:                         malc_transform_ptr\
    )\
  (expression)

#define malc_make_var_from_expression(expression, name)\
  typeof (malc_type_transform (expression)) name = \
    malc_type_transform (expression);

/*----------------------------------------------------------------------------*/
#else /* __cplusplus */

template <typename T>
struct malc_type_traits_base {
  static inline uword size      (T v) { return sizeof v; }
  static inline T     transform (T v) { return v; }
};
template<typename T> struct malc_type_traits {};

template<> struct malc_type_traits<float> :
  public malc_type_traits_base<float> {
    static const char  code = malc_type_float;
};
template<> struct malc_type_traits<double> :
  public malc_type_traits_base<double> {
    static const char  code = malc_type_double;
};
template<> struct malc_type_traits<i8> : public malc_type_traits_base<i8> {
  static const char  code = malc_type_i8;
};
template<> struct malc_type_traits<u8> : public malc_type_traits_base<u8> {
  static const char  code = malc_type_u8;
};
template<> struct malc_type_traits<i16> : public malc_type_traits_base<i16> {
  static const char  code = malc_type_i16;
};
template<> struct malc_type_traits<u16> : public malc_type_traits_base<u16> {
  static const char  code = malc_type_u16;
};
#ifdef MALC_NO_BUILTIN_COMPRESSION
  template<> struct malc_type_traits<i32> : public malc_type_traits_base<i32> {
    static const char  code = malc_type_i32;
  };
  template<> struct malc_type_traits<u32> : public malc_type_traits_base<u32> {
    static const char  code = malc_type_u32;
  };
  template<> struct malc_type_traits<i64> : public malc_type_traits_base<i64> {
    static const char  code = malc_type_i64;
  };
  template<> struct malc_type_traits<u64> : public malc_type_traits_base<u64> {
    static const char  code = malc_type_u64;
  };
#else
  template<> struct malc_type_traits<i32> {
    static const char  code = malc_type_i32;
    static inline malc_compressed_32 transform (i32 v)
    {
      return malc_get_compressed_i32 (v);
    }
  };
  template<> struct malc_type_traits<u32> {
    static const char  code = malc_type_u32;
    static inline malc_compressed_32 transform (u32 v)
    {
      return malc_get_compressed_u32 (v);
    }
  };
  template<> struct malc_type_traits<i64> {
    static const char  code = malc_type_i64;
    static inline malc_compressed_64 transform (i64 v)
    {
      return malc_get_compressed_i64 (v);
    }
  };
  template<> struct malc_type_traits<u64> {
    static const char  code = malc_type_u64;
    static inline malc_compressed_64 transform (u64 v)
    {
      return malc_get_compressed_u64 (v);
    }
  };
#endif
#ifdef MALC_NO_PTR_COMPRESSION
  template<> struct malc_type_traits<void*> :
    public malc_type_traits_base<void*> {
      static const char code = malc_type_ptr;
  };
  template<> struct malc_type_traits<malc_lit> :
    public malc_type_traits_base<malc_lit> {
      static const char  code = malc_type_lit;
  };
#else
  template<> struct malc_type_traits<void*> {
    static const char code = malc_type_ptr;
    static inline malc_compressed_ptr transform (T v)
    {
      return malc_get_compressed_ptr ((void*) v);
    }
  };
  template<> struct malc_type_traits<malc_lit> {
    static const char code = malc_type_lit;
    static inline malc_compressed_32 transform (malc_lit v)
    {
      return malc_get_compressed_ptr ((void*) v.lit);
    }
  };
#endif

template<> struct malc_type_traits<const void*> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<volatile void*> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<const volatile void*> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<void* const> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<const void* const> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<volatile void* const> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<const volatile void* const> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<malc_strcp> {
  static const char code  = malc_type_strcp;
  static inline malc_strcp transform (malc_strcp v) { return v; }
  static inline uword size (malc_strcp v)
  {
    return sizeof_member (malc_strcp, len) + v.len;
  }
};
template<> struct malc_type_traits<malc_memcp> {
  static const char  code = malc_type_memcp;
  static inline malc_memcp transform (malc_memcp v) { return v; }
  static inline uword size (malc_memcp v)
  {
    return sizeof_member (malc_memcp, size) + v.size;
  }
};

template<> struct malc_type_traits<malc_compressed_32> {
  static inline uword size (malc_compressed_32 v)
  {
    return (v.format_nibble & (u_bit (3) - 1)) + 1;
  }
};
template<> struct malc_type_traits<malc_compressed_64> {
  static inline uword size (malc_compressed_64 v)
  {
    return (v.format_nibble & (u_bit (3) - 1)) + 1;
  }
};

#include <type_traits>

#define malc_get_type_code(expression)\
  malc_type_traits< \
    typename std::remove_cv< \
      typename std::remove_reference< \
        decltype (expression) \
        >::type \
      >::type \
    >::code

#define malc_type_size(expression)\
  malc_type_traits< \
    typename std::remove_cv< \
      typename std::remove_reference< \
        decltype (expression) \
        >::type \
      >::type \
    >::size (expression)

#define malc_make_var_from_expression(expression, name)\
  auto name = malc_type_traits< \
    typename std::remove_cv< \
      typename std::remove_reference< \
        decltype (expression) \
        >::type \
      >::type \
    >::transform (expression);

#endif /* __cplusplus*/
/*----------------------------------------------------------------------------*/
/* used for testing, ignore */
#ifndef MALC_GET_MIN_SEVERITY_FNAME
  #define MALC_GET_MIN_SEVERITY_FNAME malc_get_min_severity
#endif
#ifndef MALC_LOG_FNAME
  #define MALC_LOG_FNAME malc_log
#endif
/*----------------------------------------------------------------------------*/
#define MALC_LOG_CREATE_CONST_ENTRY(sev, ...) \
  static const char pp_tokconcat(malc_const_info_, __LINE__)[] = { \
    (char) (sev), \
    /* this block builds the "info" string (the conditional is to skip*/ \
    /* the comma when empty */ \
    pp_if (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
      pp_apply( \
        malc_get_type_code, pp_comma, pp_vargs_ignore_first (__VA_ARGS__) \
        ) \
      pp_comma() \
    ) /* endif */ \
    (char) malc_end \
  }; \
  static const malc_const_entry pp_tokconcat(malc_const_entry_, __LINE__) = { \
    /* "" is prefixed to forbid compilation of non-literal format strings*/ \
    "" pp_vargs_first (__VA_ARGS__), \
    pp_tokconcat(malc_const_info_, __LINE__), \
    /* this block builds the compressed field count (0 if no vargs) */ \
    pp_if_else (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
      pp_apply( \
        malc_is_compressed, pp_plus, pp_vargs_ignore_first (__VA_ARGS__) \
        ) \
    ,/* else */ \
      0 \
    )/* endif */\
  }
/*----------------------------------------------------------------------------*/
#define MALC_LOG_PASS_TMP_VARIABLES(...) \
  pp_apply_wid (pp_vargs_second, pp_comma, __VA_ARGS__)

#define MALC_GET_TYPE_SIZE_VISITOR(expr, name)  malc_type_size (name)

#define MALC_GET_TYPE_SIZE(...) \
  pp_apply_wid (MALC_GET_TYPE_SIZE_VISITOR, pp_plus, __VA_ARGS__)
/*----------------------------------------------------------------------------*/
#define MALC_LOG_PRIVATE_IMPL(err, malc_ptr, sev, ...) \
  MALC_LOG_CREATE_CONST_ENTRY ((sev), __VA_ARGS__); \
  (err) = MALC_LOG_FNAME( \
    (malc_ptr), \
    &pp_tokconcat (malc_const_entry_, __LINE__), \
    pp_if_else (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
      MALC_GET_TYPE_SIZE (pp_vargs_ignore_first (__VA_ARGS__))\
      pp_comma() \
      MALC_LOG_PASS_TMP_VARIABLES (pp_vargs_ignore_first (__VA_ARGS__))\
    ,/*else*/\
      0\
    ) /* endif */ \
  )
/*----------------------------------------------------------------------------*/
#define MALC_LOG_DECLARE_TMP_VARIABLES(...) \
  pp_apply_wid (malc_make_var_from_expression, pp_empty, __VA_ARGS__)
/*----------------------------------------------------------------------------*/
#define MALC_LOG_IF_PRIVATE(condition, err, malc_ptr, sev, ...) \
  do { \
    if ((condition) && ((sev) >= MALC_GET_MIN_SEVERITY_FNAME ((malc_ptr)))) { \
      pp_if (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))(\
        /*A copy of the passed expressions is created, this is to avoid */\
        /*calling more than once any functions (expressions) and to do some.*/\
        /*data preprocessing. A register optimizer will find plain builtin*/\
        /*copies trivial to remove but will keep calls with side-effects.*/\
        MALC_LOG_DECLARE_TMP_VARIABLES (pp_vargs_ignore_first (__VA_ARGS__))\
      )\
      MALC_LOG_PRIVATE_IMPL ((err), (malc_ptr), (sev), __VA_ARGS__); \
    } \
    else { \
      (err) = bl_ok; \
    } \
    --(err); ++(err); /*remove unused warnings */ \
  } \
  while (0)

#define MALC_LOG_PRIVATE(err, malc_ptr, sev, ...) \
  MALC_LOG_IF_PRIVATE (1, (err), (malc_ptr), (sev), __VA_ARGS__)
/*----------------------------------------------------------------------------*/
#if !defined (MALC_STRIP_LOG_FILELINE)
  #define MALC_TO_STR(s) #s
  #define MALC_CONCAT_FILELINE(file, line) "(" file ":" MALC_TO_STR (lin) ") "
  #define malc_fileline MALC_CONCAT_FILELINE (__FILE__, __LINE__)
#else
  #define malc_fileline
#endif
/*----------------------------------------------------------------------------*/
#if !defined (MALC_GET_LOGGER_INSTANCE_FUNCNAME)
    #define MALC_GET_LOGGER_INSTANCE_FUNC get_malc_logger_instance()
#else
    #define MALC_GET_LOGGER_INSTANCE_FUNC MALC_GET_LOGGER_INSTANCE_FUNCNAME()
#endif
/*----------------------------------------------------------------------------*/
#ifdef MALC_STRIP_LOG_DEBUG
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 0
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_DEBUG"
  #endif
#endif

#ifdef MALC_STRIP_LOG_TRACE
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 1
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_TRACE"
  #endif
#endif

#ifdef MALC_STRIP_LOG_NOTICE
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 2
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_NOTICE"
  #endif
#endif

#ifdef MALC_STRIP_LOG_WARNING
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 3
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_WARNING"
  #endif
#endif

#ifdef MALC_STRIP_LOG_ERROR
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 4
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_ERROR"
  #endif
#endif

#if defined (MALC_STRIP_LOG_CRITICAL) || defined (MALC_STRIP_ALL)
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 5
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_CRITICAL"
  #endif
#endif

#if defined (MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 0 &&\
  !defined (MALC_STRIP_LOG_DEBUG)
    #define MALC_STRIP_LOG_DEBUG
#endif

#if defined(MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 1 &&\
  !defined (MALC_STRIP_LOG_TRACE)
    #define MALC_STRIP_LOG_TRACE
#endif

#if defined(MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 2 &&\
  !defined (MALC_STRIP_LOG_NOTICE)
    #define MALC_STRIP_LOG_NOTICE
#endif

#if defined(MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 3 &&\
  !defined (MALC_STRIP_LOG_WARNING)
    #define MALC_STRIP_LOG_WARNING
#endif

#if defined(MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 4 &&\
  !defined (MALC_STRIP_LOG_ERROR)
    #define MALC_STRIP_LOG_ERROR
#endif

#if defined(MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 5 &&\
  !defined (MALC_STRIP_LOG_CRITICAL)
    #define MALC_STRIP_LOG_CRITICAL
#endif
/*----------------------------------------------------------------------------*/
static inline bl_err malc_warning_silencer() { return bl_ok; }
/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_DEBUG

#define malc_debug(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_debug, __VA_ARGS__ \
    )

#define malc_debug_if(condition, err, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_debug, \
    __VA_ARGS__\
    )

#define malc_debug_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_debug, __VA_ARGS__)

#define malc_debug_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_debug, __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_DEBUG */

#define malc_debug(...)      malc_warning_silencer()
#define malc_debug_if(...)   malc_warning_silencer()
#define malc_debug_i(...)    malc_warning_silencer()
#define malc_debug_i_if(...) malc_warning_silencer()

#endif /* MALC_STRIP_LOG_DEBUG */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_TRACE

#define malc_trace(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_trace, __VA_ARGS__ \
    )

#define malc_trace_if(err, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_trace, \
    __VA_ARGS__\
    )

#define malc_trace_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_trace, __VA_ARGS__)

#define malc_trace_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_trace, __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_TRACE */

#define malc_trace(...)      malc_warning_silencer()
#define malc_trace_if(...)   malc_warning_silencer()
#define malc_trace_i(...)    malc_warning_silencer()
#define malc_trace_i_if(...) malc_warning_silencer()

#endif /* MALC_STRIP_LOG_TRACE */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_NOTICE

#define malc_notice(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_notice, __VA_ARGS__ \
    )

#define malc_notice_if(err, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_notice, \
    __VA_ARGS__\
    )

#define malc_notice_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_notice, __VA_ARGS__)

#define malc_notice_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_notice, __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_NOTICE */

#define malc_notice(...)      malc_warning_silencer()
#define malc_notice_if(...)   malc_warning_silencer()
#define malc_notice_i(...)    malc_warning_silencer()
#define malc_notice_i_if(...) malc_warning_silencer()

#endif /* MALC_STRIP_LOG_NOTICE */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_WARNING

#define malc_warning(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_warning, __VA_ARGS__ \
    )

#define malc_warning_if(err, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_warning, \
    __VA_ARGS__\
    )

#define malc_warning_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_warning, __VA_ARGS__)

#define malc_warning_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_warning, __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_WARNING */

#define malc_warning(...)      malc_warning_silencer()
#define malc_warning_if(...)   malc_warning_silencer()
#define malc_warning_i(...)    malc_warning_silencer()
#define malc_warning_i_if(...) malc_warning_silencer()

#endif /* MALC_STRIP_LOG_WARNING */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_ERROR

#define malc_error(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_error, __VA_ARGS__ \
    )

#define malc_error_if(err, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_error, \
    __VA_ARGS__\
    )

#define malc_error_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_error, __VA_ARGS__)

#define malc_error_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_error, __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_ERROR */

#define malc_error(...)      malc_warning_silencer()
#define malc_error_if(...)   malc_warning_silencer()
#define malc_error_i(...)    malc_warning_silencer()
#define malc_error_i_if(...) malc_warning_silencer()

#endif /* MALC_STRIP_LOG_ERROR */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_CRITICAL

#define malc_critical(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_critical, __VA_ARGS__ \
    )

#define malc_critical_if(err, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_critical, \
    __VA_ARGS__\
    )

#define malc_critical_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_critical, __VA_ARGS__)

#define malc_critical_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_critical, __VA_ARGS__\
    )

#else /*MALC_STRIP_LOG_CRITICAL*/

#define malc_critical(...)      malc_warning_silencer()
#define malc_critical_if(...)   malc_warning_silencer()
#define malc_critical_i(...)    malc_warning_silencer()
#define malc_critical_i_if(...) malc_warning_silencer()

#endif /*MALC_STRIP_LOG_CRITICAL*/

#endif /*include guard*/