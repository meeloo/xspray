/******************************************************************************
 *
 * Copyright (C) 2002 Jason Evans <jasone@canonware.com>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer
 *    unmodified other than the allowable addition of one or more
 *    copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 *
 * cpp macro implementation of red-black trees.  Red-black trees are difficult
 * to explain without lots of diagrams, so little attempt is made to document
 * this code.  However, an excellent discussion can be found in the following
 * book, which was used as the reference for writing this implementation:
 *
 *   Introduction to Algorithms
 *   Thomas H. Cormen, Charles E. Leiserson, and Ronald L. Rivest
 *   MIT Press (1990)
 *   ISBN 0-07-013143-0
 *
 * Some macros use a comparison function pointer, which is expected to have the
 * following prototype:
 *
 *   int (compare *)(<a_type> *a_a, <a_type> *a_b);
 *
 * Interpretation of comparision function return values:
 *
 *   -1 : a_a < a_b
 *    0 : a_a == a_b
 *    1 : a_a > a_b
 *
 * Some of the macros expand out to be quite a bit of code, so if they are
 * called in a program in more than a couple of places for a particular type, it
 * is probably a good idea to create functions that wrap the macros to keep code
 * size down.
 *
 ******************************************************************************/

/* Node structure. */
#define rb_node(a_type)							\
struct									\
{									\
    a_type *rbn_par;							\
    a_type *rbn_left;							\
    a_type *rbn_right;							\
    boolean_t rbn_red;							\
}

#define rb_node_new(a_tree, a_node, a_field)				\
    do									\
    {									\
	(a_node)->a_field.rbn_par = &(a_tree)->rbt_nil;			\
	(a_node)->a_field.rbn_left = &(a_tree)->rbt_nil;		\
	(a_node)->a_field.rbn_right = &(a_tree)->rbt_nil;		\
	(a_node)->a_field.rbn_red = FALSE;				\
    } while (0)

/* Root structure. */
#define rb_tree(a_type)							\
struct									\
{									\
    a_type *rbt_root;							\
    a_type rbt_nil;							\
}

#define rb_tree_new(a_tree, a_field)					\
    do									\
    {									\
	(a_tree)->rbt_root = &((a_tree)->rbt_nil);			\
	rb_node_new(a_tree, &(a_tree)->rbt_nil, a_field);		\
    } while (0)

#define rb_tree_nil(a_tree) (&(a_tree)->rbt_nil)

/* Operations. */
#define rb_root(a_tree) (a_tree)->rbt_root

#define rb_p_first(a_tree, a_root, a_field, r_node)			\
    do									\
    {									\
	for ((r_node) = (a_root);					\
	     (r_node)->a_field.rbn_left != &(a_tree)->rbt_nil;		\
	     (r_node) = (r_node)->a_field.rbn_left)			\
	{								\
	}								\
    } while (0)

#define rb_p_last(a_tree, a_root, a_field, r_node)			\
    do									\
    {									\
	for ((r_node) = (a_root);					\
	     (r_node)->a_field.rbn_right != &(a_tree)->rbt_nil; 	\
	     (r_node) = (r_node)->a_field.rbn_right)			\
	{								\
	}								\
    } while (0)

#define rb_first(a_tree, a_field, r_node)				\
    rb_p_first(a_tree, rb_root(a_tree), a_field, r_node)

#define rb_last(a_tree, a_field, r_node)				\
    rb_p_last(a_tree, rb_root(a_tree), a_field, r_node)

#define rb_next(a_tree, a_node, a_type, a_field, r_node)		\
    do									\
    {									\
	if ((a_node)->a_field.rbn_right != &(a_tree)->rbt_nil)		\
	{								\
	    rb_p_first(a_tree, (a_node)->a_field.rbn_right, a_field,	\
		     r_node);						\
	}								\
	else								\
	{								\
	    a_type *t = (a_node);					\
	    (r_node) = (a_node)->a_field.rbn_par;			\
	    while ((r_node) != &(a_tree)->rbt_nil			\
		   && t == (r_node)->a_field.rbn_right)			\
	    {								\
		t = (r_node);						\
		(r_node) = (r_node)->a_field.rbn_par;			\
	    }								\
	}								\
    } while (0)

#define rb_prev(a_tree, a_node, a_type, a_field, r_node)		\
    do									\
    {									\
	if ((a_node)->a_field.rbn_left != &(a_tree)->rbt_nil)		\
	{								\
	    rb_p_last(a_tree, (a_node)->a_field.rbn_left, a_field,	\
		    r_node);						\
	}								\
	else								\
	{								\
	    a_type *t = (a_node);					\
	    (r_node) = (a_node)->a_field.rbn_par;			\
	    while ((r_node) != &(a_tree)->rbt_nil			\
		   && t == (r_node)->a_field.rbn_left)			\
	    {								\
		t = (r_node);						\
		(r_node) = (r_node)->a_field.rbn_par;			\
	    }								\
	}								\
    } while (0)

/* a_key is always the first argument to a_comp. */
#define rb_search(a_tree, a_key, a_comp, a_field, r_node)		\
    do									\
    {									\
	int t;								\
	(r_node) = (a_tree)->rbt_root;					\
	while ((r_node) != &(a_tree)->rbt_nil				\
	       && (t = (a_comp)((a_key), (r_node))) != 0)		\
	{								\
	    if (t == -1)						\
	    {								\
		(r_node) = (r_node)->a_field.rbn_left;			\
	    }								\
	    else							\
	    {								\
		(r_node) = (r_node)->a_field.rbn_right;			\
	    }								\
	}								\
    } while (0)

/* Find a match if it exists.  Otherwise, find the next greater node, if one
 * exists. */
#define rb_nsearch(a_tree, a_key, a_comp, a_type, a_field, r_node)	\
    do									\
    {									\
	int t;								\
	(r_node) = (a_tree)->rbt_root;					\
	while ((r_node) != &(a_tree)->rbt_nil				\
	       && (t = (a_comp)((a_key), (r_node))) != 0)		\
	{								\
	    if (t == -1)						\
	    {								\
		if ((r_node)->a_field.rbn_left == &(a_tree)->rbt_nil)	\
		{							\
		    break;						\
		}							\
		(r_node) = (r_node)->a_field.rbn_left;			\
	    }								\
	    else							\
	    {								\
		if ((r_node)->a_field.rbn_right == &(a_tree)->rbt_nil)	\
		{							\
		    a_type *n = (r_node);				\
		    (r_node) = (r_node)->a_field.rbn_par;		\
		    while ((r_node) != &(a_tree)->rbt_nil		\
			   && n == (r_node)->a_field.rbn_right)		\
		    {							\
			n = (r_node);					\
			(r_node) = (r_node)->a_field.rbn_par;		\
		    }							\
		    break;						\
		}							\
		(r_node) = (r_node)->a_field.rbn_right;			\
	    }								\
	}								\
    } while (0)

#define rb_p_left_rotate(a_tree, a_node, a_type, a_field)		\
    do									\
    {									\
	a_type *t = (a_node)->a_field.rbn_right;			\
	(a_node)->a_field.rbn_right = t->a_field.rbn_left;		\
	if (t->a_field.rbn_left != &(a_tree)->rbt_nil)			\
	{								\
	    t->a_field.rbn_left->a_field.rbn_par = (a_node);		\
	}								\
	t->a_field.rbn_par = (a_node)->a_field.rbn_par;			\
	if ((a_node)->a_field.rbn_par == &(a_tree)->rbt_nil) 		\
	{								\
	    (a_tree)->rbt_root = t;					\
	}								\
	else if ((a_node)						\
		 == (a_node)->a_field.rbn_par->a_field.rbn_left)	\
	{								\
	    (a_node)->a_field.rbn_par->a_field.rbn_left = t;		\
	}								\
	else								\
	{								\
	    (a_node)->a_field.rbn_par->a_field.rbn_right = t;		\
	}								\
	t->a_field.rbn_left = (a_node);					\
	(a_node)->a_field.rbn_par = t;					\
    } while (0)

#define rb_p_right_rotate(a_tree, a_node, a_type, a_field)		\
    do									\
    {									\
	a_type *t = (a_node)->a_field.rbn_left;				\
	(a_node)->a_field.rbn_left = t->a_field.rbn_right;		\
	if (t->a_field.rbn_right != &(a_tree)->rbt_nil)			\
	{								\
	    t->a_field.rbn_right->a_field.rbn_par = (a_node);		\
	}								\
	t->a_field.rbn_par = (a_node)->a_field.rbn_par;			\
	if ((a_node)->a_field.rbn_par == &(a_tree)->rbt_nil)		\
	{								\
	    (a_tree)->rbt_root = t;					\
	}								\
	else if ((a_node)						\
		 == (a_node)->a_field.rbn_par->a_field.rbn_right)	\
	{								\
	    (a_node)->a_field.rbn_par->a_field.rbn_right = t;		\
	}								\
	else								\
	{								\
	    (a_node)->a_field.rbn_par->a_field.rbn_left = t;		\
	}								\
	t->a_field.rbn_right = (a_node);				\
	(a_node)->a_field.rbn_par = t;					\
    } while (0)

/* a_node is always the first argument to a_comp. */
#define rb_insert(a_tree, a_node, a_comp, a_type, a_field)		\
    do									\
    {									\
	/* Insert. */							\
	a_type *x = &(a_tree)->rbt_nil;					\
	a_type *y = (a_tree)->rbt_root;					\
	int c = 0;							\
	while (y != &(a_tree)->rbt_nil)					\
	{								\
	    x = y;							\
	    c = (a_comp)((a_node), y);					\
	    if (c == -1)						\
	    {								\
		y = y->a_field.rbn_left;				\
	    }								\
	    else							\
	    {								\
		y = y->a_field.rbn_right;				\
	    }								\
	}								\
	(a_node)->a_field.rbn_par = x;					\
	if (x == &(a_tree)->rbt_nil)					\
	{								\
	    (a_tree)->rbt_root = (a_node);				\
	}								\
	else if (c == -1)						\
	{								\
	    x->a_field.rbn_left = (a_node);				\
	}								\
	else								\
	{								\
	    x->a_field.rbn_right = (a_node);				\
	}								\
	/* Fix up. */							\
	x = (a_node);							\
	x->a_field.rbn_red = TRUE;					\
	while (x != (a_tree)->rbt_root					\
	       && x->a_field.rbn_par->a_field.rbn_red)			\
	{								\
	    y = x->a_field.rbn_par;					\
	    if (y == y->a_field.rbn_par->a_field.rbn_left)		\
	    {								\
		y = y->a_field.rbn_par->a_field.rbn_right;		\
		if (y->a_field.rbn_red)					\
		{							\
		    x->a_field.rbn_par->a_field.rbn_red = FALSE;	\
		    y->a_field.rbn_red = FALSE;				\
		    x->a_field.rbn_par->a_field.rbn_par			\
			->a_field.rbn_red = TRUE;			\
		    x = x->a_field.rbn_par->a_field.rbn_par;		\
		}							\
		else							\
		{							\
		    if (x == x->a_field.rbn_par->a_field.rbn_right)	\
		    {							\
			x = x->a_field.rbn_par;				\
			rb_p_left_rotate(a_tree, x, a_type, a_field);	\
		    }							\
		    x->a_field.rbn_par->a_field.rbn_red = FALSE;	\
		    x->a_field.rbn_par->a_field.rbn_par			\
			->a_field.rbn_red = TRUE;			\
		    x = x->a_field.rbn_par->a_field.rbn_par;		\
		    rb_p_right_rotate(a_tree, x, a_type, a_field);	\
		}							\
	    }								\
	    else							\
	    {								\
		y = y->a_field.rbn_par->a_field.rbn_left;		\
		if (y->a_field.rbn_red)					\
		{							\
		    x->a_field.rbn_par->a_field.rbn_red = FALSE;	\
		    y->a_field.rbn_red = FALSE;				\
		    x->a_field.rbn_par->a_field.rbn_par			\
			->a_field.rbn_red = TRUE;			\
		    x = x->a_field.rbn_par->a_field.rbn_par;		\
		}							\
		else							\
		{							\
		    if (x == x->a_field.rbn_par->a_field.rbn_left)	\
		    {							\
			x = x->a_field.rbn_par;				\
			rb_p_right_rotate(a_tree, x, a_type, a_field);	\
		    }							\
		    x->a_field.rbn_par->a_field.rbn_red = FALSE;	\
		    x->a_field.rbn_par->a_field.rbn_par			\
			->a_field.rbn_red = TRUE;			\
		    x = x->a_field.rbn_par->a_field.rbn_par;		\
		    rb_p_left_rotate(a_tree, x, a_type, a_field);	\
		}							\
	    }								\
	}								\
	(a_tree)->rbt_root->a_field.rbn_red = FALSE;			\
    } while (0)

#define rb_remove(a_tree, a_node, a_type, a_field)			\
    do									\
    {									\
	boolean_t fixup;						\
	a_type *x, *y;							\
	if ((a_node)->a_field.rbn_left == &(a_tree)->rbt_nil		\
	    || (a_node)->a_field.rbn_right == &(a_tree)->rbt_nil)	\
	{								\
	    y = (a_node);						\
	}								\
	else								\
	{								\
	    rb_next(a_tree, a_node, a_type, a_field, y);		\
	}								\
	if (y->a_field.rbn_left != &(a_tree)->rbt_nil)			\
	{								\
	    x = y->a_field.rbn_left;					\
	}								\
	else								\
	{								\
	    x = y->a_field.rbn_right;					\
	}								\
	x->a_field.rbn_par = y->a_field.rbn_par;			\
	if (y->a_field.rbn_par == &(a_tree)->rbt_nil)			\
	{								\
	    (a_tree)->rbt_root = x;					\
	}								\
	else if (y == y->a_field.rbn_par->a_field.rbn_left)		\
	{								\
	    y->a_field.rbn_par->a_field.rbn_left = x;			\
	}								\
	else								\
	{								\
	    y->a_field.rbn_par->a_field.rbn_right = x;			\
	}								\
	if (y->a_field.rbn_red == FALSE)				\
	{								\
	    fixup = TRUE;						\
	}								\
	else								\
	{								\
	    fixup = FALSE;						\
	}								\
	if (y != (a_node))						\
	{								\
	    /* Splice y into a_node's location. */			\
	    y->a_field.rbn_par = (a_node)->a_field.rbn_par;		\
	    y->a_field.rbn_left = (a_node)->a_field.rbn_left;		\
	    y->a_field.rbn_right = (a_node)->a_field.rbn_right;		\
	    y->a_field.rbn_red = (a_node)->a_field.rbn_red;		\
	    if (y->a_field.rbn_par != &(a_tree)->rbt_nil)		\
	    {								\
		if (y->a_field.rbn_par->a_field.rbn_left == (a_node))	\
		{							\
		    y->a_field.rbn_par->a_field.rbn_left = y;		\
		}							\
		else							\
		{							\
		    y->a_field.rbn_par->a_field.rbn_right = y;		\
		}							\
	    }								\
	    else							\
	    {								\
		(a_tree)->rbt_root = y;					\
	    }								\
	    y->a_field.rbn_right->a_field.rbn_par = y;			\
	    y->a_field.rbn_left->a_field.rbn_par = y;			\
	}								\
	rb_node_new(a_tree, a_node, a_field);				\
	if (fixup)							\
	{								\
	    /* Fix up. */						\
	    a_type *v, *w;						\
	    while (x != (a_tree)->rbt_root				\
		   && x->a_field.rbn_red == FALSE)			\
	    {								\
		if (x == x->a_field.rbn_par->a_field.rbn_left)		\
		{							\
		    w = x->a_field.rbn_par->a_field.rbn_right;		\
		    if (w->a_field.rbn_red)				\
		    {							\
			w->a_field.rbn_red = FALSE;			\
			v = x->a_field.rbn_par;				\
			v->a_field.rbn_red = TRUE;			\
			rb_p_left_rotate(a_tree, v, a_type, a_field);	\
			w = x->a_field.rbn_par->a_field.rbn_right;	\
		    }							\
		    if (w->a_field.rbn_left->a_field.rbn_red		\
			== FALSE					\
			&& w->a_field.rbn_right->a_field.rbn_red	\
			== FALSE)					\
		    {							\
			w->a_field.rbn_red = TRUE;			\
			x = x->a_field.rbn_par;				\
		    }							\
		    else						\
		    {							\
			if (w->a_field.rbn_right->a_field.rbn_red	\
			     == FALSE)					\
			{						\
			    w->a_field.rbn_left->a_field.rbn_red	\
				= FALSE;				\
			    w->a_field.rbn_red = TRUE;			\
			    rb_p_right_rotate(a_tree, w, a_type,	\
					      a_field);			\
			    w = x->a_field.rbn_par->a_field.rbn_right;	\
			}						\
			w->a_field.rbn_red				\
			    = x->a_field.rbn_par->a_field.rbn_red;	\
			x->a_field.rbn_par->a_field.rbn_red = FALSE;	\
			w->a_field.rbn_right->a_field.rbn_red = FALSE;	\
			v = x->a_field.rbn_par;				\
			rb_p_left_rotate(a_tree, v, a_type, a_field);	\
			break;						\
		    }							\
		}							\
		else							\
		{							\
		    w = x->a_field.rbn_par->a_field.rbn_left;		\
		    if (w->a_field.rbn_red)				\
		    {							\
			w->a_field.rbn_red = FALSE;			\
			v = x->a_field.rbn_par;				\
			v->a_field.rbn_red = TRUE;			\
			rb_p_right_rotate(a_tree, v, a_type, a_field);	\
			w = x->a_field.rbn_par->a_field.rbn_left;	\
		    }							\
		    if (w->a_field.rbn_right->a_field.rbn_red		\
			== FALSE					\
			&& w->a_field.rbn_left->a_field.rbn_red		\
			== FALSE)					\
		    {							\
			w->a_field.rbn_red = TRUE;			\
			x = x->a_field.rbn_par;				\
		    }							\
		    else						\
		    {							\
			if (w->a_field.rbn_left->a_field.rbn_red	\
			     == FALSE)					\
			{						\
			    w->a_field.rbn_right->a_field.rbn_red	\
				= FALSE;				\
			    w->a_field.rbn_red = TRUE;			\
			    rb_p_left_rotate(a_tree, w, a_type,		\
					     a_field);			\
			    w = x->a_field.rbn_par->a_field.rbn_left;	\
			}						\
			w->a_field.rbn_red				\
			    = x->a_field.rbn_par->a_field.rbn_red;	\
			x->a_field.rbn_par->a_field.rbn_red = FALSE;	\
			w->a_field.rbn_left->a_field.rbn_red = FALSE;	\
			v = x->a_field.rbn_par;				\
			rb_p_right_rotate(a_tree, v, a_type, a_field);	\
			break;						\
		    }							\
		}							\
	    }								\
	    x->a_field.rbn_red = FALSE;					\
	}								\
    } while (0)
