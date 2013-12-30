/*
 * Copyright (C) 2012 OpenHeadend S.A.R.L.
 *
 * Authors: Christophe Massiot
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/** @file
 * @short Upipe buffer handling for picture managers
 * This file defines the picture-specific API to access buffers.
 */

#ifndef _UPIPE_UBUF_PIC_H_
/** @hidden */
#define _UPIPE_UBUF_PIC_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <upipe/ubuf.h>

#include <stdint.h>
#include <stdbool.h>

/** @This returns a new ubuf from a picture allocator.
 *
 * @param mgr management structure for this ubuf type
 * @param hsize horizontal size in pixels
 * @param vsize vertical size in lines
 * @return pointer to ubuf or NULL in case of failure
 */
static inline struct ubuf *ubuf_pic_alloc(struct ubuf_mgr *mgr,
                                          int hsize, int vsize)
{
    return ubuf_alloc(mgr, UBUF_ALLOC_PICTURE, hsize, vsize);
}

/** @This returns the sizes of the picture ubuf.
 *
 * @param ubuf pointer to ubuf
 * @param hsize_p reference written with the horizontal size of the picture
 * if not NULL
 * @param vsize_p reference written with the vertical size of the picture
 * if not NULL
 * @param macropixel_p reference written with the number of pixels in a
 * macropixel if not NULL
 * @return false in case of error
 */
static inline bool ubuf_pic_size(struct ubuf *ubuf,
                                 size_t *hsize_p, size_t *vsize_p,
                                 uint8_t *macropixel_p)
{
    return ubase_err_check(ubuf_control(ubuf, UBUF_SIZE_PICTURE,
                                        hsize_p, vsize_p, macropixel_p));
}

/** @This iterates on picture planes chroma types. Start by initializing
 * *chroma_p to NULL. If *chroma_p is NULL after running this function, there
 * are no more planes in this picture. Otherwise the string pointed to by
 * *chroma_p remains valid until the ubuf picture manager is deallocated.
 *
 * @param ubuf pointer to ubuf
 * @param chroma_p reference written with chroma type of the next plane
 * @return false in case of error
 */
static inline bool ubuf_pic_plane_iterate(struct ubuf *ubuf,
                                          const char **chroma_p)
{
    return ubase_err_check(ubuf_control(ubuf, UBUF_ITERATE_PICTURE_PLANE,
                                        chroma_p));
}

/** @This returns the sizes of a plane of the picture ubuf.
 *
 * @param ubuf pointer to ubuf
 * @param chroma chroma type (see chroma reference)
 * @param stride_p reference written with the offset between lines, in octets,
 * if not NULL
 * @param hsub_p reference written with the horizontal subsamping for this plane
 * if not NULL
 * @param vsub_p reference written with the vertical subsamping for this plane
 * if not NULL
 * @param macropixel_size_p reference written with the size of a macropixel in
 * octets for this plane if not NULL
 * @return false in case of error
 */
static inline bool ubuf_pic_plane_size(struct ubuf *ubuf, const char *chroma,
                                       size_t *stride_p,
                                       uint8_t *hsub_p, uint8_t *vsub_p,
                                       uint8_t *macropixel_size_p)

{
    return ubase_err_check(ubuf_control(ubuf, UBUF_SIZE_PICTURE_PLANE, chroma,
                                        stride_p, hsub_p, vsub_p,
                                        macropixel_size_p));
}

/** @internal @This checks the offset and size parameters of a lot of functions,
 * and transforms them into absolute offset and size.
 *
 * @param ubuf pointer to ubuf
 * @param chroma chroma type (see chroma reference)
 * @param hoffset_p reference to horizontal offset of the picture area wanted
 * in the whole picture, negative values start from the end of lines, in pixels
 * (before dividing by macropixel and hsub)
 * @param voffset_p reference to vertical offset of the picture area wanted
 * in the whole picture, negative values start from the last line, in lines
 * (before dividing by vsub)
 * @param hsize_p reference to number of pixels wanted per line, or -1 for
 * until the end of the line
 * @param vsize_p reference to number of lines wanted in the picture area,
 * or -1 for until the last line (may be NULL)
 * @return false when the parameters are invalid
 */
static inline bool ubuf_pic_plane_check_offset(struct ubuf *ubuf,
                                               const char *chroma,
                                               int *hoffset_p, int *voffset_p,
                                               int *hsize_p, int *vsize_p)
{
    size_t ubuf_hsize, ubuf_vsize;
    uint8_t macropixel;
    if (unlikely(!ubuf_pic_size(ubuf, &ubuf_hsize, &ubuf_vsize, &macropixel) ||
                 *hoffset_p > (int)ubuf_hsize || *voffset_p > (int)ubuf_vsize ||
                 *hoffset_p + *hsize_p > (int)ubuf_hsize ||
                 *voffset_p + *vsize_p > (int)ubuf_vsize))
        return false;
    if (*hoffset_p < 0)
        *hoffset_p += ubuf_hsize;
    if (*voffset_p < 0)
        *voffset_p += ubuf_vsize;
    if (*hsize_p == -1)
        *hsize_p = ubuf_hsize - *hoffset_p;
    if (*vsize_p == -1)
        *vsize_p = ubuf_vsize - *voffset_p;
    if (unlikely(*hoffset_p % macropixel || *hsize_p % macropixel))
        return false;

    uint8_t hsub, vsub;
    if (unlikely(!ubuf_pic_plane_size(ubuf, chroma, NULL, &hsub, &vsub, NULL) ||
                 *hoffset_p % hsub || *hsize_p % hsub ||
                 *voffset_p % vsub || *vsize_p % vsub))
        return false;
    return true;
}

/** @This returns a read-only pointer to the buffer space. You must call
 * @ref ubuf_pic_plane_unmap when you're done with the pointer.
 *
 * @param ubuf pointer to ubuf
 * @param chroma chroma type (see chroma reference)
 * @param hoffset horizontal offset of the picture area wanted in the whole
 * picture, negative values start from the end of lines, in pixels (before
 * dividing by macropixel and hsub)
 * @param voffset vertical offset of the picture area wanted in the whole
 * picture, negative values start from the last line, in lines (before dividing
 * by vsub)
 * @param hsize number of pixels wanted per line, or -1 for until the end of
 * the line (before dividing by macropixel and hsub)
 * @param vsize number of lines wanted in the picture area, or -1 for until the
 * last line (before deviding by vsub)
 * @param buffer_p reference written with a pointer to buffer space if not NULL
 * @return false in case of error
 */
static inline bool ubuf_pic_plane_read(struct ubuf *ubuf, const char *chroma,
                                       int hoffset, int voffset,
                                       int hsize, int vsize,
                                       const uint8_t **buffer_p)
{
    if (unlikely(!ubuf_pic_plane_check_offset(ubuf, chroma, &hoffset, &voffset,
                                              &hsize, &vsize)))
        return false;
    return ubase_err_check(ubuf_control(ubuf, UBUF_READ_PICTURE_PLANE, chroma,
                                        hoffset, voffset, hsize, vsize,
                                        buffer_p));
}

/** @This returns a writable pointer to the buffer space, if the ubuf is not
 * shared. You must call @ref ubuf_pic_plane_unmap when you're done with the
 * pointer.
 *
 * @param ubuf pointer to ubuf
 * @param chroma chroma type (see chroma reference)
 * @param hoffset horizontal offset of the picture area wanted in the whole
 * picture, negative values start from the end of lines, in pixels (before
 * dividing by macropixel and hsub)
 * @param voffset vertical offset of the picture area wanted in the whole
 * picture, negative values start from the last line, in lines (before dividing
 * by vsub)
 * @param hsize number of pixels wanted per line, or -1 for until the end of
 * the line
 * @param vsize number of lines wanted in the picture area, or -1 for until the
 * last line
 * @param buffer_p reference written with a pointer to buffer space if not NULL
 * @return false in case of error
 */
static inline bool ubuf_pic_plane_write(struct ubuf *ubuf, const char *chroma,
                                        int hoffset, int voffset,
                                        int hsize, int vsize,
                                        uint8_t **buffer_p)
{
    if (unlikely(!ubuf_pic_plane_check_offset(ubuf, chroma, &hoffset, &voffset,
                                              &hsize, &vsize)))
        return false;
    return ubase_err_check(ubuf_control(ubuf, UBUF_WRITE_PICTURE_PLANE, chroma,
                                        hoffset, voffset, hsize, vsize,
                                        buffer_p));
}

/** @This marks the buffer space as being currently unused, and the pointer
 * will be invalid until the next time the ubuf is mapped.
 *
 * @param ubuf pointer to ubuf
 * @param chroma chroma type (see chroma reference)
 * @return false in case of error
 */
static inline bool ubuf_pic_plane_unmap(struct ubuf *ubuf, const char *chroma,
                                        int hoffset, int voffset,
                                        int hsize, int vsize)
{
    if (unlikely(!ubuf_pic_plane_check_offset(ubuf, chroma, &hoffset, &voffset,
                                              &hsize, &vsize)))
        return false;
    return ubase_err_check(ubuf_control(ubuf, UBUF_UNMAP_PICTURE_PLANE, chroma,
                                        hoffset, voffset, hsize, vsize));
}

/** @internal @This checks the skip and new_size parameters of a lot of
 * resizing functions, and transforms them.
 *
 * @param ubuf pointer to ubuf
 * @param hskip_p reference to number of pixels to skip at the beginning of
 * each line (if < 0, extend the picture leftwards)
 * @param vskip_p reference to number of lines to skip at the beginning of
 * the picture (if < 0, extend the picture upwards)
 * @param new_hsize_p reference to final horizontal size of the buffer,
 * in pixels (if set to -1, keep same line ends)
 * @param new_vsize_p reference to final vertical size of the buffer,
 * in lines (if set to -1, keep same last line)
 * @param ubuf_hsize_p filled in with the total horizontal size of the ubuf
 * (may be NULL)
 * @param ubuf_vsize_p filled in with the total vertical size of the ubuf
 * (may be NULL)
 * @param macropixel_p filled in with the number of pixels in a macropixel
 * (may be NULL)
 * @return false when the parameters are invalid
 */
static inline bool ubuf_pic_check_resize(struct ubuf *ubuf,
                                         int *hskip_p, int *vskip_p,
                                         int *new_hsize_p, int *new_vsize_p,
                                         size_t *ubuf_hsize_p,
                                         size_t *ubuf_vsize_p,
                                         uint8_t *macropixel_p)
{
    size_t ubuf_hsize, ubuf_vsize;
    uint8_t macropixel;
    if (unlikely(!ubuf_pic_size(ubuf, &ubuf_hsize, &ubuf_vsize, &macropixel) ||
                 *hskip_p > (int)ubuf_hsize || *vskip_p > (int)ubuf_vsize))
        return false;
    if (*new_hsize_p == -1)
        *new_hsize_p = ubuf_hsize - *hskip_p;
    if (*new_vsize_p == -1)
        *new_vsize_p = ubuf_vsize - *vskip_p;
    if (unlikely(*new_hsize_p < -*hskip_p || *new_vsize_p < -*vskip_p))
        return false;
    if (unlikely((*hskip_p < 0 && -*hskip_p % macropixel) ||
                 (*hskip_p > 0 && *hskip_p % macropixel) ||
                 *new_hsize_p % macropixel))
        return false;
    if (ubuf_hsize_p != NULL)
        *ubuf_hsize_p = ubuf_hsize;
    if (ubuf_vsize_p != NULL)
        *ubuf_vsize_p = ubuf_vsize;
    if (macropixel_p != NULL)
        *macropixel_p = macropixel;
    return true;
}

/** @This resizes a picture ubuf, if possible. This will only work if:
 * @list
 * @item the ubuf is only shrinked in one or both directions, or
 * @item the relevant low-level buffer is not shared with another ubuf and the
 * picture manager allows to grow the buffer (ie. prepend/append have been
 * correctly specified at allocation, or reallocation is allowed)
 * @end list
 *
 * Should this fail, @ref ubuf_pic_replace may be used to achieve the same goal
 * with an extra buffer copy.
 *
 * @param ubuf pointer to ubuf
 * @param hskip number of pixels to skip at the beginning of each line (if < 0,
 * extend the picture leftwards)
 * @param vskip number of lines to skip at the beginning of the picture (if < 0,
 * extend the picture upwards)
 * @param new_hsize final horizontal size of the buffer, in pixels (if set
 * to -1, keep same line ends)
 * @param new_vsize final vertical size of the buffer, in lines (if set
 * to -1, keep same last line)
 * @return false in case of error, or if the ubuf is shared, or if the operation
 * is not possible
 */
static inline bool ubuf_pic_resize(struct ubuf *ubuf,
                                   int hskip, int vskip,
                                   int new_hsize, int new_vsize)
{
    if (unlikely(!ubuf_pic_check_resize(ubuf, &hskip, &vskip,
                                        &new_hsize, &new_vsize,
                                        NULL, NULL, NULL)))
        return false;
    return ubase_err_check(ubuf_control(ubuf, UBUF_RESIZE_PICTURE,
                                        hskip, vskip, new_hsize, new_vsize));
}

/** @This copies a picture ubuf to a newly allocated ubuf, and doesn't deal
 * with the old ubuf or a dictionary.
 *
 * @param mgr management structure for this ubuf type
 * @param ubuf pointer to ubuf to copy
 * @param hskip number of pixels to skip at the beginning of each line (if < 0,
 * extend the picture leftwards)
 * @param vskip number of lines to skip at the beginning of the picture (if < 0,
 * extend the picture upwards)
 * @param new_hsize final horizontal size of the buffer, in pixels (if set
 * to -1, keep same line ends)
 * @param new_vsize final vertical size of the buffer, in lines (if set
 * to -1, keep same last line)
 * @return pointer to newly allocated ubuf or NULL in case of error
 */
static inline struct ubuf *ubuf_pic_copy(struct ubuf_mgr *mgr,
                                         struct ubuf *ubuf,
                                         int hskip, int vskip,
                                         int new_hsize, int new_vsize)
{
    size_t ubuf_hsize, ubuf_vsize;
    uint8_t macropixel;
    if (unlikely(!ubuf_pic_check_resize(ubuf, &hskip, &vskip,
                                        &new_hsize, &new_vsize,
                                        &ubuf_hsize, &ubuf_vsize, &macropixel)))
        return NULL;

    struct ubuf *new_ubuf = ubuf_pic_alloc(mgr, new_hsize, new_vsize);
    if (unlikely(new_ubuf == NULL))
        return NULL;

    uint8_t new_macropixel;
    if (unlikely(!ubuf_pic_size(new_ubuf, NULL, NULL, &new_macropixel) ||
                 new_macropixel != macropixel))
        goto ubuf_pic_copy_err;

    int extract_hoffset, extract_hskip;
    if (hskip < 0) {
        extract_hoffset = -hskip;
        extract_hskip = 0;
    } else {
        extract_hoffset = 0;
        extract_hskip = hskip;
    }
    int extract_hsize =
        new_hsize - extract_hoffset <= ubuf_hsize - extract_hskip ?
        new_hsize - extract_hoffset : ubuf_hsize - extract_hskip;

    int extract_voffset, extract_vskip;
    if (vskip < 0) {
        extract_voffset = -vskip;
        extract_vskip = 0;
    } else {
        extract_voffset = 0;
        extract_vskip = vskip;
    }
    int extract_vsize =
        new_vsize - extract_voffset <= ubuf_vsize - extract_vskip ?
        new_vsize - extract_voffset : ubuf_vsize - extract_vskip;

    const char *chroma = NULL;
    while (ubuf_pic_plane_iterate(ubuf, &chroma) && chroma != NULL) {
        size_t stride;
        uint8_t hsub, vsub, macropixel_size;
        if (unlikely(!ubuf_pic_plane_size(ubuf, chroma, &stride,
                                          &hsub, &vsub, &macropixel_size)))
            goto ubuf_pic_copy_err;

        size_t new_stride;
        uint8_t new_hsub, new_vsub, new_macropixel_size;
        if (unlikely(!ubuf_pic_plane_size(new_ubuf, chroma, &new_stride,
                                          &new_hsub, &new_vsub,
                                          &new_macropixel_size)))
            goto ubuf_pic_copy_err;

        if (unlikely(hsub != new_hsub || vsub != new_vsub ||
                     macropixel_size != new_macropixel_size))
            goto ubuf_pic_copy_err;

        uint8_t *new_buffer;
        const uint8_t *buffer;
        if (unlikely(!ubuf_pic_plane_write(new_ubuf, chroma,
                                           extract_hoffset, extract_voffset,
                                           extract_hsize, extract_vsize,
                                           &new_buffer)))
            goto ubuf_pic_copy_err;
        if (unlikely(!ubuf_pic_plane_read(ubuf, chroma,
                                          extract_hskip, extract_vskip,
                                          extract_hsize, extract_vsize,
                                          &buffer))) {
            ubuf_pic_plane_unmap(new_ubuf, chroma,
                                 extract_hoffset, extract_voffset,
                                 extract_hsize, extract_vsize);
            goto ubuf_pic_copy_err;
        }

        int plane_hsize = extract_hsize / hsub / macropixel * macropixel_size;
        int plane_vsize = extract_vsize / vsub;

        for (int i = 0; i < plane_vsize; i++) {
            memcpy(new_buffer, buffer, plane_hsize);
            new_buffer += new_stride;
            buffer += stride;
        }

        bool ret = ubuf_pic_plane_unmap(new_ubuf, chroma,
                                        extract_hoffset, extract_voffset,
                                        extract_hsize, extract_vsize);
        if (unlikely(!ubuf_pic_plane_unmap(ubuf, chroma,
                                           extract_hskip, extract_vskip,
                                           extract_hsize, extract_vsize) ||
                     !ret))
            goto ubuf_pic_copy_err;
    }
    return new_ubuf;

ubuf_pic_copy_err:
    ubuf_free(new_ubuf);
    return NULL;
}

/** @This copies part of a ubuf to a newly allocated ubuf, and replaces the
 * old ubuf with the new ubuf.
 *
 * @param mgr management structure for this ubuf type
 * @param ubuf_p reference to a pointer to ubuf to replace with a new picture
 * ubuf
 * @param hskip number of pixels to skip at the beginning of each line (if < 0,
 * extend the picture leftwards)
 * @param vskip number of lines to skip at the beginning of the picture (if < 0,
 * extend the picture upwards)
 * @param new_hsize final horizontal size of the buffer, in pixels (if set
 * to -1, keep same line ends)
 * @param new_vsize final vertical size of the buffer, in lines (if set
 * to -1, keep same last line)
 * @return false in case of allocation error
 */
static inline bool ubuf_pic_replace(struct ubuf_mgr *mgr,
                                    struct ubuf **ubuf_p,
                                    int hskip, int vskip,
                                    int new_hsize, int new_vsize)
{
    struct ubuf *new_ubuf = ubuf_pic_copy(mgr, *ubuf_p, hskip, vskip,
                                          new_hsize, new_vsize);
    if (unlikely(new_ubuf == NULL))
        return false;

    ubuf_free(*ubuf_p);
    *ubuf_p = new_ubuf;
    return true;
}

/** @This clears (part of) the specified plane, depending on plane type
 * and size (set U/V chroma to 0x80 instead of 0 for instance)
 *
 * @param ubuf pointer to ubuf
 * @param chroma chroma type (see chroma reference)
 * @param hoffset horizontal offset of the picture area wanted in the whole
 * picture, negative values start from the end of lines, in pixels (before
 * dividing by macropixel and hsub)
 * @param voffset vertical offset of the picture area wanted in the whole
 * picture, negative values start from the last line, in lines (before dividing
 * by vsub)
 * @param hsize number of pixels wanted per line, or -1 for until the end of
 * the line
 * @param vsize number of lines wanted in the picture area, or -1 for until the
 * last line
 * @return false if chroma not known or in case of error
 */
bool ubuf_pic_plane_clear(struct ubuf *ubuf, const char *chroma,
                          int hoffset, int voffset,
                          int hsize, int vsize);

/** @This clears (part of) the specified picture, depending on plane type
 * and size (set U/V chroma to 0x80 instead of 0 for instance)
 *
 * @param ubuf pointer to ubuf
 * @param hoffset horizontal offset of the picture area wanted in the whole
 * picture, negative values start from the end of lines, in pixels (before
 * dividing by macropixel and hsub)
 * @param voffset vertical offset of the picture area wanted in the whole
 * picture, negative values start from the last line, in lines (before dividing
 * by vsub)
 * @param hsize number of pixels wanted per line, or -1 for until the end of
 * the line
 * @param vsize number of lines wanted in the picture area, or -1 for until the
 * last line
 * @return false if chroma not known or in case of error
 */
bool ubuf_pic_clear(struct ubuf *ubuf, int hoffset, int voffset,
                                       int hsize, int vsize);

#ifdef __cplusplus
}
#endif
#endif
