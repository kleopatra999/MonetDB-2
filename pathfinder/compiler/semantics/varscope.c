/* -*- c-basic-offset:4; c-indentation-style:"k&r"; indent-tabs-mode:nil -*- */

/**
 * @file
 *
 * Check variable scoping in abstract syntax tree.
 *
 * The abstract syntax tree is traversed recursively. With the help of
 * a stack-type environment, variable scoping is checked. On any occurence of
 *  - a @c flwr, @c some or @c every node, or a function declaration
 *    node, the current variable environment is saved and restored 
 *    after the node has been processed.  The variables are bound only within 
 *    the rightmost subtree of the node.
 *
 *  - a @c typeswitch node, the leftmost child is processed (which may
 *    not use any variable that wasn't seen so far). If the typeswitch
 *    contains an @c as clause, the variable is pushed onto the stack
 *    (which also creates a new PFvar_t struct). The @c cases and
 *    @c default subtrees are processed with the new variable in scope.
 *    The variable is popped off the stack before returning from the
 *    typeswitch expression (if there is an @c as clause).
 *
 *  - a variable binding (can be either a @c let node or a @c bind node
 *    that is used for @c for, @c some and @c every), A new variable is
 *    created with the help of PFnew_var. The pointer to the correspondig
 *    @c struct is pushed onto the variabel environment stack.
 *    If we find a variable of the same name on the stack (i.e., in
 *    scope), issue a warning about variable reuse.  Note that
 *    a @c bind node may bring into scope up to two variables 
 *    (positional variables in @c for).  
 *
 *  - a variable usage, the stack is scanned top down for the first
 *    occurence of a variable with the same name. If the variable is
 *    found, this is the correct in-scope variable. If the search is not
 *    successful, scoping rules have been violated by the user. In this
 *    case we push an error message onto the stack and set a
 *    @c scoping_failed flag to stop processing the query after this
 *    phase. The scope checking, however, is continued to possibly find
 *    more scoping rule violations and report them to the user.
 *    A typical rule violation is variable reuse.
 *
 * Within this scoping phase, for all variables that occur in the user
 * query, the pointer to a QName (see #PFpnode_t, #PFsem_t) is replaced
 * by a pointer to a PFvar_t struct. This struct again contains the
 * QName of the variable and uniquely identifies the variable. Later
 * occurencies of the same variable point to the same PFvar_t struct.
 * To indicate successful scoping, the type of abstract syntax tree node
 * is lifted from p_varref (unscoped) to p_var (scoped).
 *
 *
 * Copyright Notice:
 * -----------------
 *
 *  The contents of this file are subject to the MonetDB Public
 *  License Version 1.0 (the "License"); you may not use this file
 *  except in compliance with the License. You may obtain a copy of
 *  the License at http://monetdb.cwi.nl/Legal/MonetDBLicense-1.0.html
 *
 *  Software distributed under the License is distributed on an "AS
 *  IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 *  implied. See the License for the specific language governing
 *  rights and limitations under the License.
 *
 *  The Original Code is the ``Pathfinder'' system. The Initial
 *  Developer of the Original Code is the Database & Information
 *  Systems Group at the University of Konstanz, Germany. Portions
 *  created by U Konstanz are Copyright (C) 2000-2004 University
 *  of Konstanz. All Rights Reserved.
 *
 *  Contributors:
 *          Torsten Grust <torsten.grust@uni-konstanz.de>
 *          Maurice van Keulen <M.van.Keulen@bigfoot.com>
 *          Jens Teubner <jens.teubner@uni-konstanz.de>
 *
 * $Id$
 */

#include <assert.h>

#include "pathfinder.h"
#include "varscope.h"

#include "variable.h"
/* PFscope_t */
#include "scope.h"
#include "oops.h"

/** Create variable environment used for scoping */
static PFscope_t *var_env;

static PFvar_t *find_var (PFqname_t);

/**
 * If a violation of scoping rules is found during processing, this flag
 * is set to true. We still keep on processing to possibly find more rule
 * violations and report them all to the user.
 */
static bool scoping_failed = false;

/**
 * Push a new variable onto the variable environment stack.
 * @param varname Name of the variable to push onto the stack. 
 * @return Status code as described in pathfinder/oops.c
 */
void
push (PFpnode_t *n)
{
    PFvar_t *var;
    PFqname_t *varname;

    /* create new variable */
    assert (n && n->kind == p_varref);

    varname = &(n->sem.qname);
    assert (varname);

    if (! (var = PFnew_var (*varname)))
        PFoops (OOPS_OUTOFMEM, 
                "allocation of new variable failed");

    /* If we find a variable of the same name on the stack (i.e., in scope),
     * issue a warning (the user might mistake an XQuery clause like 
     * `let $x := $x + 1' for an imperative variable update). 
     */
    if (PFscope_lookup (var_env, *varname))
        PFinfo_loc (OOPS_WARN_VARREUSE, n->loc, "$%s", PFqname_str (*varname));

    /* register variable in environment */
    PFscope_into (var_env, *varname, var);

    /* initialize fields of varref node n */
    n->sem.var = var;
    n->kind = p_var;
}

/**
 * Traverse the stack top-down and return the PFvar_t pointer that
 * corresponds to the given variable name using the scoping rules.
 * @param varname The variable name to look for
 * @return Pointer to the corresponding PFvar_t struct or @c NULL
 *   if the variable was not found.
 */
static PFvar_t *
find_var (PFqname_t varname)
{
    return ((PFvar_t *) PFscope_lookup (var_env, varname));
}

/**
 * Traverse through abstract syntax tree and do actual variable scope
 * checking work. For some node types, action code is executed as
 * described above.
 *
 * @param n The current abstract syntax tree node
 */
void
scope (PFpnode_t *n)
{
    unsigned int child;     /* Iterate over children */
    PFvar_t     *var;

    assert (n);

    switch (n->kind) {
    case p_varref:  /* Variable usage */
        /*
         * Traverse stack topdown and find the first variable whose name
         * matches the current variable. If we find it, we store the
         * information in the current parse tree node. If not (NULL is
         * returned), scoping rules have been violated. Query processing
         * has to stop after this scoping phase. We do, however, not abort
         * processing here, but keep on looking for more rule violations
         * to report them all to the user.
         */
        var = find_var (n->sem.qname);
        
        if (!var) {
            PFinfo_loc (OOPS_UNKNOWNVAR, n->loc,
                        "$%s", PFqname_str (n->sem.qname));
            scoping_failed = true;
        }
        
        n->sem.var = var;
        n->kind = p_var;
        
        break;
        
    case p_flwr:     
        /*                       flwr
         *                      / | | \
         *                 binds  o p  e
         *
         * (1) save current variable environment 
         * (2) process variable bindings 
         * (3) process o `order by', p `where', and e `return' clauses
         * (4) restore variable environment
         */
        
        /* (1) */
        PFscope_open (var_env);
        
        /* (2) */
        scope (n->child[0]);
        
        /* (3) */
        scope (n->child[1]);
        scope (n->child[2]);
        scope (n->child[3]);
        
        /* (4) */
        PFscope_close (var_env);
        
        break;
        
    case p_some:      
    case p_every:     
        /*                       some/every
         *                         /   \
         *                     binds    e
         *
         * (1) save current variable environment 
         * (2) process variable bindings 
         * (3) process quantifier body e `satifies' clause
         * (4) restore variable environment
         */
        
        /* (1) */
        PFscope_open (var_env);
        
        /* (2) */
        scope (n->child[0]);
        
        /* (3) */
        scope (n->child[1]);
        
        /* (4) */
        PFscope_close (var_env);
        
        break;
        
    case p_fun_decl:  
        /*                      fun_decl
         *                      /  |  \
         *                 params  t   e
         *
         * (1) save current variable environment 
         * (2) process function parameters
         * (3) process return type t and function body e
         * (4) restore variable environment
         */
        
        /* (1) */
        PFscope_open (var_env);
        
        /* (2) */
        scope (n->child[0]);
        
        /* (3) */
        scope (n->child[1]);
        scope (n->child[2]);
        
        /* (4) */
        PFscope_close (var_env);
        
        break;
        
    case p_bind:     
        /*                 bind
         *                /| |\          for $v as t at $i in e
         *               t i v e         some $v as t satisfies e
         *
         * (i may be nil: no positional vars for some/every)
         *
         * (1) process e (v, i not yet visible in e)
         * (2) bring v into scope
         * (3) bring i into scope (if present)
         */
      
      
        /* (1) */
        scope (n->child[3]);

        /* (2) */
        assert (n->child[2] && (n->child[2]->kind == p_varref));

        push (n->child[2]);
        
        /* (3) */
        assert (n->child[1]);

        if (n->child[1]->kind == p_varref)
            push (n->child[1]);

        break;

    case p_let:  
        /*                 let
         *                / | \          let $v as t := e 
         *               t  v  e
         *
         * (1) process e (v not yet visible in e)
         * (2) bring v into scope
         */
      
        /* (1) */
        scope (n->child[2]);
      
        /* (2) */
        assert (n->child[1] && (n->child[1]->kind == p_varref));

        push (n->child[1]);

        break;

    case p_param:    /* function parameter */
        /* Abstract syntax tree layout:
         *
         *                param                
         *                 / \        define function (..., t v, ...)
         *                t   v
         */
        assert (n->child[1] && (n->child[1]->kind == p_varref));

        push (n->child[1]);

        break;

    case p_case:          /* branch of a typeswitch expression */
        /* Abstract syntax tree layout:
         *
         *                 case
         *                 / | \             case t v return e
         *                t  v  e
         */
      
        /* occurrence of a branch variable is optional */
        assert (n->child[1]);

        /* visibility of branch variable is branch-local only */
        PFscope_open (var_env);

        if (n->child[1]->kind == p_varref)
            push (n->child[1]);

        /* visit the case branch e itself */
        assert (n->child[2]);
        
        scope (n->child[2]);

        PFscope_close (var_env);

        break;

    case p_typeswitch:    /* typeswitch expression */
        /* Abstract syntax tree layout:
         *
         *             typeswitch
         *              / |  | \      typeswitch (e) cs default $v return e' 
         *             e  cs v  e'
         */

        /* Process typeswitch operand and cases */
        assert (n->child[0]);
        assert (n->child[1]);

        scope (n->child[0]);
        scope (n->child[1]);
      
        /* occurrence of the default branch variable is optional */
        assert (n->child[2]);

        /* visibility of branch variable is branch-local only */
        PFscope_open (var_env);

        if (n->child[2]->kind == p_varref)
            push (n->child[2]);
      
        /* visit the default branch e' itself */
        assert (n->child[3]);
        
        scope (n->child[3]);
      
        PFscope_close (var_env);

        break;

    default:
        /*
         * For all other cases just traverse the whole tree recursively.
         */
        for (child = 0; (child < PFPNODE_MAXCHILD) && n->child[child]; child++)
            scope (n->child[child]);

        break;
    }

}

/**
 * Check variable scoping for the parse tree.
 *
 * Scoping rules are checked recursively. In all nodes that reference
 * variables, the qname in the semantic content of the node is replaced
 * by a pointer to a PFvar_t struct (var). If scoping rules are violated,
 * a message is pushed onto the error stack for each violation and the
 * function returns an error code. An error can also be returned due to
 * other errors, like out of memory errors.
 * @param root Pointer to the parse tree root node
 */
void
PFvarscope (PFpnode_t * root)
{
    var_env = PFscope ();

    scope (root);

    if (scoping_failed)
        PFoops (OOPS_UNKNOWNVAR,
                "erroneous variable references reported above");
}


/* vim:set shiftwidth=4 expandtab: */
