/*
 * Copyright (c) 2013 Franco Fichtner <franco@packetwerk.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef PEAK_PAGE_H
#define PEAK_PAGE_H

#define PAGE_DECLARE(name, type, num)					\
struct name {								\
	struct name *moar;						\
	unsigned int reserved;						\
	unsigned int count;						\
	type list[num];							\
}

#define PAGE_MIN(elm, page, root) ({					\
	(page) = root;							\
	if ((page)) {							\
		(elm) = (page)->list;					\
	} else {							\
		(elm) = NULL;						\
	}								\
	elm;								\
})

#define PAGE_MAX(elm, page, root) ({					\
	elm = PAGE_MIN(elm, page, root);				\
	if (elm) {							\
		while ((page)->moar) {					\
			page = (page)->moar;				\
		}							\
		elm = &(page)->list[(page)->count - 1];			\
	}								\
	elm;								\
})

#define PAGE_NEXT(elm, page) ({						\
	if ((elm) < &(page)->list[(page)->count - 1]) {			\
		++(elm);						\
	} else {							\
		(page) = (page)->moar;					\
		PAGE_MIN(elm, page, page);				\
	}								\
	elm;								\
})

#define PAGE_FOREACH(x, y, root)					\
	for (PAGE_MIN(x, y, root); (x); PAGE_NEXT(x, y))

#define PAGE_FIND(root, x, cmp_fun) ({					\
	typeof(&(root)->list[0]) _found;				\
	typeof(root) _page;						\
	unsigned int _i;						\
	PAGE_MIN(_found, _page, root);					\
	while (_page) {							\
		if (cmp_fun(x, &_page->list[0]) < 0) {			\
			_found = NULL;					\
			break;						\
		}							\
		if (cmp_fun(x, &_page->list[_page->count - 1]) > 0) {	\
			PAGE_MIN(_found, _page, _page->moar);		\
			continue;					\
		}							\
		_found = NULL;						\
		for (_i = 0; _i < _page->count; ++_i) {			\
			if (!cmp_fun(x, &_page->list[_i])) {		\
				_found = &_page->list[_i];		\
				break;					\
			}						\
		}							\
		break;							\
	}								\
	_found;								\
})

#define PAGE_INSERT(root, x, cmp_fun, alloc_fun, alloc_arg) ({		\
	typeof(&(root)->list[0]) _elem, _ret = NULL;			\
	typeof(root) _new, _page;					\
	unsigned int _i;						\
	PAGE_MIN(_elem, _page, root);					\
	while (_page) {							\
		if (cmp_fun(x, _elem) < 0) {				\
			break;						\
		}							\
		if (cmp_fun(x, &_page->list[_page->count - 1]) > 0) {	\
			PAGE_MIN(_elem, _page, _page->moar);		\
			continue;					\
		}							\
		_elem = NULL;						\
		for (_i = 0; _i < _page->count; ++_i) {			\
			const int _cmp = cmp_fun(x, &_page->list[_i]);	\
			if (!_cmp) {					\
				_ret = _elem = &_page->list[_i];	\
				break;					\
			} else if(_cmp < 0) {				\
				_elem = &_page->list[_i];		\
				break;					\
			}						\
		}							\
		break;							\
	}								\
	if (!_elem) {							\
		_elem = PAGE_MAX(_elem, _page, root);			\
		if (_elem) {						\
			/*						\
			 * So we went through the list, but the		\
			 * last element is still smaller than		\
			 * the next one, so we have to point one	\
			 * position further.  The other parts of	\
			 * the implementation are aware of this.  ;)	\
			 */						\
			++_elem;					\
		}							\
	}								\
	if (_ret) {							\
		_elem = NULL;						\
	} else if (!_elem) {						\
		_new = alloc_fun(alloc_arg, sizeof(*_new));		\
		if (likely(_new)) {					\
			_elem = &_new->list[0];				\
			(root) = _page = _new;				\
			_new->moar = NULL;				\
			_new->count = 0;				\
		}							\
	} else if (_page->count == lengthof(_page->list)) {		\
		_new = alloc_fun(alloc_arg, sizeof(*_new));		\
		if (likely(_new)) {					\
			const unsigned int _pos = _elem - _page->list;	\
			const unsigned int _split =			\
			    (lengthof(_page->list) + 1) / 2;		\
			/* hook up new page */				\
			_new->moar = _page->moar;			\
			_page->moar = _new;				\
			/* biased split */				\
			if (_pos >= _split) {				\
				_new->count = _page->count - _split;	\
				_page->count = _split;			\
			} else {					\
				_page->count = _page->count - _split;	\
				_new->count = _split;			\
			}						\
			memmove(_new->list, &_page->list[_page->count],	\
			    sizeof(*_elem) * _new->count);		\
			/* pick the right page */			\
			if (_pos >= _split) {				\
				_elem =	&_new->list[_pos - _split];	\
				_page = _new;				\
			}						\
			/* fallthrough for insert */			\
		} else {						\
			_elem = NULL;					\
		}							\
	}								\
	if (_elem) {							\
		const unsigned int _pos = _elem - _page->list;		\
		if (_pos < _page->count) {				\
			memmove(_elem + 1, _elem, sizeof(*_elem) *	\
			    (_page->count - _pos));			\
		}							\
		++_page->count;						\
		*_elem = *(x);						\
		_ret = _elem;						\
	}								\
	_ret;								\
})

#define PAGE_COLLAPSE(root, free_fun, free_arg) do {			\
	typeof(root) _page = root;					\
	while (_page) {							\
		typeof(root) _temp = _page->moar;			\
		free_fun(free_arg, _page);				\
		_page = _temp;						\
	}								\
	root = NULL;							\
} while (0)

#endif /* !PEAK_PAGE_H */
