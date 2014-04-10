/********************************************************************\
 * guile-util.c -- utility functions for using guile for GnuCash    *
 * Copyright (C) 1999 Linas Vepstas                                 *
 * Copyright (C) 2000 Dave Peticolas                                *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, write to the Free Software      *
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.        *
\********************************************************************/

#include "config.h"

#include <string.h>

#include "global-options.h"
#include "gnc-engine-util.h"
#include "engine-helpers.h"
#include "glib-helpers.h"
#include "guile-util.h"
#include "messages.h"

#include <g-wrap-wct.h>
#include <libguile.h>
#include "guile-mappings.h"

/* This static indicates the debugging module this .o belongs to.  */
static short module = MOD_GUILE;


struct _setters
{
  SCM split_scm_account_guid;
  SCM split_scm_memo;
  SCM split_scm_action;
  SCM split_scm_reconcile_state;
  SCM split_scm_amount;
  SCM split_scm_value;

  SCM trans_scm_date;
  SCM trans_scm_num;
  SCM trans_scm_description;
  SCM trans_scm_notes;
  SCM trans_scm_append_split_scm;
} setters;

struct _getters
{
  SCM split_scm_memo;
  SCM split_scm_action;
  SCM split_scm_amount;
  SCM split_scm_value;

  SCM trans_scm_split_scms;
  SCM trans_scm_split_scm;
  SCM trans_scm_other_split_scm;

  SCM debit_string;
  SCM credit_string;
} getters;

struct _predicates
{
  SCM is_split_scm;
  SCM is_trans_scm;
} predicates;


static void
initialize_scm_functions()
{
  static gboolean scm_funcs_inited = FALSE;

  if (scm_funcs_inited)
    return;

  setters.split_scm_account_guid =
    scm_c_eval_string("gnc:split-scm-set-account-guid");
  setters.split_scm_memo = scm_c_eval_string("gnc:split-scm-set-memo");
  setters.split_scm_action = scm_c_eval_string("gnc:split-scm-set-action");
  setters.split_scm_reconcile_state =
    scm_c_eval_string("gnc:split-scm-set-reconcile-state");
  setters.split_scm_amount = scm_c_eval_string("gnc:split-scm-set-amount");
  setters.split_scm_value = scm_c_eval_string("gnc:split-scm-set-value");

  setters.trans_scm_date = scm_c_eval_string("gnc:transaction-scm-set-date-posted");
  setters.trans_scm_num = scm_c_eval_string("gnc:transaction-scm-set-num");
  setters.trans_scm_description =
    scm_c_eval_string("gnc:transaction-scm-set-description");
  setters.trans_scm_notes = scm_c_eval_string("gnc:transaction-scm-set-notes");
  setters.trans_scm_append_split_scm =
    scm_c_eval_string("gnc:transaction-scm-append-split-scm");

  getters.split_scm_memo = scm_c_eval_string("gnc:split-scm-get-memo");
  getters.split_scm_action = scm_c_eval_string("gnc:split-scm-get-action");
  getters.split_scm_amount = scm_c_eval_string("gnc:split-scm-get-amount");
  getters.split_scm_value = scm_c_eval_string("gnc:split-scm-get-value");

  getters.trans_scm_split_scms =
    scm_c_eval_string("gnc:transaction-scm-get-split-scms");
  getters.trans_scm_split_scm =
    scm_c_eval_string("gnc:transaction-scm-get-split-scm");
  getters.trans_scm_other_split_scm =
    scm_c_eval_string("gnc:transaction-scm-get-other-split-scm");

  getters.debit_string = scm_c_eval_string("gnc:get-debit-string");
  getters.credit_string = scm_c_eval_string("gnc:get-credit-string");

  predicates.is_split_scm = scm_c_eval_string("gnc:split-scm?");
  predicates.is_trans_scm = scm_c_eval_string("gnc:transaction-scm?");

  scm_funcs_inited = TRUE;
}


/********************************************************************\
 * gnc_guile_call1_to_string                                        *
 *   returns the malloc'ed string returned by the guile function    *
 *   or NULL if it can't be retrieved                               *
 *                                                                  *
 * Args: func - the guile function to call                          *
 *       arg  - the single function argument                        *
 * Returns: malloc'ed char * or NULL                                *
\********************************************************************/
char *
gnc_guile_call1_to_string(SCM func, SCM arg)
{
  SCM value;

  if (SCM_PROCEDUREP(func))
  {
    value = scm_call_1(func, arg);

    if (SCM_STRINGP(value))
      return gh_scm2newstr(value, NULL);
    else
    {
      PERR("bad value\n");
    }
  }
  else
  {
    PERR("not a procedure\n");
  }

  return NULL;
}


/********************************************************************\
 * gnc_guile_call1_symbol_to_string                                 *
 *   returns the malloc'ed string returned by the guile function    *
 *   or NULL if it can't be retrieved. The return value of the      *
 *   function should be a symbol.                                   *
 *                                                                  *
 * Args: func - the guile function to call                          *
 *       arg  - the single function argument                        *
 * Returns: malloc'ed char * or NULL                                *
\********************************************************************/
char *
gnc_guile_call1_symbol_to_string(SCM func, SCM arg)
{
  SCM value;

  if (SCM_PROCEDUREP(func))
  {
    value = scm_call_1(func, arg);

    if (SCM_SYMBOLP(value))
      return gh_symbol2newstr(value, NULL);
    else
    {
      PERR("bad value\n");
    }
  }
  else
  {
    PERR("not a procedure\n");
  }

  return NULL;
}


/********************************************************************\
 * gnc_guile_call1_to_procedure                                     *
 *   returns the SCM handle to the procedure returned by the guile  *
 *   function, or SCM_UNDEFINED if it couldn't be retrieved.        *
 *                                                                  *
 * Args: func - the guile function to call                          *
 *       arg  - the single function argument                        *
 * Returns: SCM function handle or SCM_UNDEFINED                    *
\********************************************************************/
SCM
gnc_guile_call1_to_procedure(SCM func, SCM arg)
{
  SCM value;

  if (SCM_PROCEDUREP(func))
  {
    value = scm_call_1(func, arg);

    if (SCM_PROCEDUREP(value))
      return value;
    else
    {
      PERR("bad value\n");
    }
  }
  else
  {
    PERR("not a procedure\n");
  }

  return SCM_UNDEFINED;
}


/********************************************************************\
 * gnc_guile_call1_to_list                                          *
 *   returns the SCM handle to the list returned by the guile       *
 *   function, or SCM_UNDEFINED if it couldn't be retrieved.        *
 *                                                                  *
 * Args: func - the guile function to call                          *
 *       arg  - the single function argument                        *
 * Returns: SCM list handle or SCM_UNDEFINED                        *
\********************************************************************/
SCM
gnc_guile_call1_to_list(SCM func, SCM arg)
{
  SCM value;

  if (SCM_PROCEDUREP(func))
  {
    value = scm_call_1(func, arg);

    if (SCM_LISTP(value))
      return value;
    else
    {
      PERR("bad value\n");
    }
  }
  else
  {
    PERR("not a procedure\n");
  }

  return SCM_UNDEFINED;
}


/********************************************************************\
 * gnc_guile_call1_to_vector                                        *
 *   returns the SCM handle to the vector returned by the guile     *
 *   function, or SCM_UNDEFINED if it couldn't be retrieved.        *
 *                                                                  *
 * Args: func - the guile function to call                          *
 *       arg  - the single function argument                        *
 * Returns: SCM vector handle or SCM_UNDEFINED                      *
\********************************************************************/
SCM
gnc_guile_call1_to_vector(SCM func, SCM arg)
{
  SCM value;

  if (SCM_PROCEDUREP(func))
  {
    value = scm_call_1(func, arg);

    if (SCM_VECTORP(value))
      return value;
    else
    {
      PERR("bad value\n");
    }
  }
  else
  {
    PERR("not a procedure\n");
  }

  return SCM_UNDEFINED;
}


/********************************************************************\
  gnc_scm_lookup                    

    returns the SCM binding associated with the given symbol function,
    or SCM_UNDEFINED if it couldn't be retrieved.

    Don't use this to get hold of symbols that are considered private
    to a given module unless the C code you're writing is considered
    part of that module.

  Args: 

    module - where to lookup the symbol, something like "ice-9 debug"
    symbol - what to look up.

  Returns: value bound to the symbol, if any.
\********************************************************************/

#if 0

  ************ NOT TESTED YET **************

SCM
gnc_scm_lookup(const char *module, const char *symbol)
{
#if defined(SCM_GUILE_MAJOR_VERSION) && \
    (SCM_GUILE_MAJOR_VERSION > 0) && (SCM_GUILE_MINOR_VERSION > 4)
  
  SCM scm_module = scm_c_resolve_module(module);
  SCM value = scm_c_module_lookup(scm_module, symbol);
  return value;
#else
  
  gchar *in_guard_str;
  gchar *thunk_str;
  SCM in_guard;
  SCM thunk;
  SCM out_guard;
  SCM result;

  in_guard_str =
    g_strdup_printf("(lambda () (set-current-module (resolve-module (%s))))",
                    module);

  thunk_str = g_strdup_printf("(lambda () (eval '%s))", symbol);

  in_guard = scm_c_eval_string(in_guard_str);
  thunk = scm_c_eval_string(thunk_str);
  out_guard = scm_c_eval_string("(let ((cm (current-module)))"
                          "  (lambda () (set-current-module cm)))");

  result = scm_dynamic_wind(in_guard, thunk, out_guard);

  g_free(in_guard_str);
  g_free(thunk_str);

  return result;
#endif
}

#endif

/********************************************************************\
 * gnc_copy_split                                                   *
 *   returns a scheme representation of a split. If the split is    *
 *   NULL, SCM_UNDEFINED is returned.                               *
 *                                                                  *
 * Args: split             - the split to copy                      *
 *       use_cut_semantics - if TRUE, copy is for a 'cut' operation *
 * Returns: SCM representation of split or SCM_UNDEFINED            *
\********************************************************************/
SCM
gnc_copy_split(Split *split, gboolean use_cut_semantics)
{
  static SCM split_type = SCM_UNDEFINED;
  SCM func;
  SCM arg;

  if (split == NULL)
    return SCM_UNDEFINED;

  func = scm_c_eval_string("gnc:split->split-scm");
  if (!SCM_PROCEDUREP(func))
    return SCM_UNDEFINED;

  if(split_type == SCM_UNDEFINED) {
    split_type = scm_c_eval_string("<gnc:Split*>");
    /* don't really need this - types are bound globally anyway. */
    if(split_type != SCM_UNDEFINED) scm_protect_object(split_type);
  }

  arg = gw_wcp_assimilate_ptr(split, split_type);

  return scm_call_2(func, arg, SCM_BOOL(use_cut_semantics));
}


/********************************************************************\
 * gnc_copy_split_scm_onto_split                                    *
 *   copies a scheme representation of a split onto an actual split.*
 *                                                                  *
 * Args: split_scm - the scheme representation of a split           *
 *       split     - the split to copy onto                         *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_copy_split_scm_onto_split(SCM split_scm, Split *split,
                              GNCBook * book)
{
  static SCM split_type = SCM_UNDEFINED;
  SCM result;
  SCM func;
  SCM arg;

  if (split_scm == SCM_UNDEFINED)
    return;

  if (split == NULL)
    return;

  g_return_if_fail (book);

  func = scm_c_eval_string("gnc:split-scm?");
  if (!SCM_PROCEDUREP(func))
    return;

  result = scm_call_1(func, split_scm);
  if (!SCM_NFALSEP(result))
    return;

  func = scm_c_eval_string("gnc:split-scm-onto-split");
  if (!SCM_PROCEDUREP(func))
    return;

  if(split_type == SCM_UNDEFINED) {
    split_type = scm_c_eval_string("<gnc:Split*>");
    /* don't really need this - types are bound globally anyway. */
    if(split_type != SCM_UNDEFINED) scm_protect_object(split_type);
  }

  arg = gw_wcp_assimilate_ptr(split, split_type);

  scm_call_3(func, split_scm, arg, gnc_book_to_scm (book));
}


/********************************************************************\
 * gnc_is_split_scm                                                 *
 *   returns true if the scm object is a scheme split               *
 *                                                                  *
 * Args: scm - a scheme object                                      *
 * Returns: true if scm is a scheme split                           *
\********************************************************************/
gboolean
gnc_is_split_scm(SCM scm)
{
  initialize_scm_functions();

  return SCM_NFALSEP(scm_call_1(predicates.is_split_scm, scm));
}


/********************************************************************\
 * gnc_is_trans_scm                                                 *
 *   returns true if the scm object is a scheme transaction         *
 *                                                                  *
 * Args: scm - a scheme object                                      *
 * Returns: true if scm is a scheme transaction                     *
\********************************************************************/
gboolean
gnc_is_trans_scm(SCM scm)
{
  initialize_scm_functions();

  return SCM_NFALSEP(scm_call_1(predicates.is_trans_scm, scm));
}


/********************************************************************\
 * gnc_split_scm_set_account                                        *
 *   set the account of a scheme representation of a split.         *
 *                                                                  *
 * Args: split_scm - the scheme split                               *
 *       account   - the account to set                             *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_split_scm_set_account(SCM split_scm, Account *account)
{
  char *guid_string;
  SCM arg;

  initialize_scm_functions();

  if (!gnc_is_split_scm(split_scm))
    return;
  if (account == NULL)
    return;

  guid_string = guid_to_string(xaccAccountGetGUID(account));
  if (guid_string == NULL)
    return;

  arg = scm_makfrom0str(guid_string);

  scm_call_2(setters.split_scm_account_guid, split_scm, arg);

  g_free(guid_string);
}


/********************************************************************\
 * gnc_split_scm_set_memo                                           *
 *   set the memo of a scheme representation of a split.            *
 *                                                                  *
 * Args: split_scm - the scheme split                               *
 *       memo      - the memo to set                                *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_split_scm_set_memo(SCM split_scm, const char *memo)
{
  SCM arg;

  initialize_scm_functions();

  if (!gnc_is_split_scm(split_scm))
    return;
  if (memo == NULL)
    return;

  arg = scm_makfrom0str(memo);

  scm_call_2(setters.split_scm_memo, split_scm, arg);
}


/********************************************************************\
 * gnc_split_scm_set_action                                         *
 *   set the action of a scheme representation of a split.          *
 *                                                                  *
 * Args: split_scm - the scheme split                               *
 *       action    - the action to set                              *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_split_scm_set_action(SCM split_scm, const char *action)
{
  SCM arg;

  initialize_scm_functions();

  if (!gnc_is_split_scm(split_scm))
    return;
  if (action == NULL)
    return;

  arg = scm_makfrom0str(action);

  scm_call_2(setters.split_scm_action, split_scm, arg);
}


/********************************************************************\
 * gnc_split_scm_set_reconcile_state                                *
 *   set the reconcile state of a scheme split.                     *
 *                                                                  *
 * Args: split_scm       - the scheme split                         *
 *       reconcile_state - the reconcile state to set               *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_split_scm_set_reconcile_state(SCM split_scm, char reconcile_state)
{
  SCM arg;

  initialize_scm_functions();

  if (!gnc_is_split_scm(split_scm))
    return;

  arg = SCM_MAKE_CHAR(reconcile_state);

  scm_call_2(setters.split_scm_reconcile_state, split_scm, arg);
}


/********************************************************************\
 * gnc_split_scm_set_amount                                         *
 *   set the amount of a scheme split                               *
 *                                                                  *
 * Args: split_scm - the scheme split                               *
 *       amount    - the amount to set                              *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_split_scm_set_amount(SCM split_scm, gnc_numeric amount)
{
  SCM arg;

  initialize_scm_functions();

  if (!gnc_is_split_scm(split_scm))
    return;

  arg = gnc_numeric_to_scm(amount);
  scm_call_2(setters.split_scm_amount, split_scm, arg);
}


/********************************************************************\
 * gnc_split_scm_set_value                                          *
 *   set the value of a scheme split                                *
 *                                                                  *
 * Args: split_scm - the scheme split                               *
 *       value     - the value to set                               *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_split_scm_set_value(SCM split_scm, gnc_numeric value)
{
  SCM arg;

  initialize_scm_functions();

  if (!gnc_is_split_scm(split_scm))
    return;

  arg = gnc_numeric_to_scm(value);
  scm_call_2(setters.split_scm_value, split_scm, arg);
}


/********************************************************************\
 * gnc_split_scm_get_memo                                           *
 *   return the newly allocated memo of a scheme split, or NULL.    *
 *                                                                  *
 * Args: split_scm - the scheme split                               *
 * Returns: newly allocated memo string                             *
\********************************************************************/
char *
gnc_split_scm_get_memo(SCM split_scm)
{
  SCM result;

  initialize_scm_functions();

  if (!gnc_is_split_scm(split_scm))
    return NULL;

  result = scm_call_1(getters.split_scm_memo, split_scm);
  if (!SCM_STRINGP(result))
    return NULL;

  return gh_scm2newstr(result, NULL);
}


/********************************************************************\
 * gnc_split_scm_get_action                                         *
 *   return the newly allocated action of a scheme split, or NULL.  *
 *                                                                  *
 * Args: split_scm - the scheme split                               *
 * Returns: newly allocated action string                           *
\********************************************************************/
char *
gnc_split_scm_get_action(SCM split_scm)
{
  SCM result;

  initialize_scm_functions();

  if (!gnc_is_split_scm(split_scm))
    return NULL;

  result = scm_call_1(getters.split_scm_action, split_scm);
  if (!SCM_STRINGP(result))
    return NULL;

  return gh_scm2newstr(result, NULL);
}


/********************************************************************\
 * gnc_split_scm_get_amount                                         *
 *   return the amount of a scheme split                            *
 *                                                                  *
 * Args: split_scm - the scheme split                               *
 * Returns: amount of scheme split                                  *
\********************************************************************/
gnc_numeric
gnc_split_scm_get_amount(SCM split_scm)
{
  SCM result;

  initialize_scm_functions();

  if (!gnc_is_split_scm(split_scm))
    return gnc_numeric_zero ();

  result = scm_call_1(getters.split_scm_amount, split_scm);
  if (!gnc_numeric_p(result))
    return gnc_numeric_zero ();

  return gnc_scm_to_numeric(result);
}


/********************************************************************\
 * gnc_split_scm_get_value                                          *
 *   return the value of a scheme split                             *
 *                                                                  *
 * Args: split_scm - the scheme split                               *
 * Returns: value of scheme split                                   *
\********************************************************************/
gnc_numeric
gnc_split_scm_get_value(SCM split_scm)
{
  SCM result;

  initialize_scm_functions();

  if (!gnc_is_split_scm(split_scm))
    return gnc_numeric_zero ();

  result = scm_call_1(getters.split_scm_value, split_scm);
  if (!gnc_numeric_p(result))
    return gnc_numeric_zero ();

  return gnc_scm_to_numeric(result);
}


/********************************************************************\
 * gnc_copy_trans                                                   *
 *   returns a scheme representation of a transaction. If the       *
 *   transaction is NULL, SCM_UNDEFINED is returned.                *
 *                                                                  *
 * Args: trans             - the transaction to copy                *
 *       use_cut_semantics - if TRUE, copy is for a 'cut' operation *
 * Returns: SCM representation of transaction or SCM_UNDEFINED      *
\********************************************************************/
SCM
gnc_copy_trans(Transaction *trans, gboolean use_cut_semantics)
{
  static SCM trans_type = SCM_UNDEFINED;
  SCM func;
  SCM arg;

  if (trans == NULL)
    return SCM_UNDEFINED;

  func = scm_c_eval_string("gnc:transaction->transaction-scm");
  if (!SCM_PROCEDUREP(func))
    return SCM_UNDEFINED;

  if(trans_type == SCM_UNDEFINED) {
    trans_type = scm_c_eval_string("<gnc:Transaction*>");
    /* don't really need this - types are bound globally anyway. */
    if(trans_type != SCM_UNDEFINED) scm_protect_object(trans_type);
  }

  arg = gw_wcp_assimilate_ptr(trans, trans_type);

  return scm_call_2(func, arg, SCM_BOOL(use_cut_semantics));
}


/********************************************************************\
 * gnc_copy_trans_scm_onto_trans                                    *
 *   copies a scheme representation of a transaction onto           *
 *   an actual transaction.                                         *
 *                                                                  *
 * Args: trans_scm - the scheme representation of a transaction     *
 *       trans     - the transaction to copy onto                   *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_copy_trans_scm_onto_trans(SCM trans_scm, Transaction *trans,
                              gboolean do_commit, GNCBook *book)
{
  gnc_copy_trans_scm_onto_trans_swap_accounts(trans_scm, trans, NULL, NULL,
                                              do_commit, book);
}


/********************************************************************\
 * gnc_copy_trans_scm_onto_trans_swap_accounts                      *
 *   copies a scheme representation of a transaction onto           *
 *   an actual transaction. If guid_1 and guid_2 are not NULL,      *
 *   the account guids of the splits are swapped accordingly.       *
 *                                                                  *
 * Args: trans_scm - the scheme representation of a transaction     *
 *       trans     - the transaction to copy onto                   *
 *       guid_1    - account guid to swap with guid_2               *
 *       guid_2    - account guid to swap with guid_1               *
 *       do_commit - whether to commit the edits                    *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_copy_trans_scm_onto_trans_swap_accounts(SCM trans_scm,
                                            Transaction *trans,
                                            const GUID *guid_1,
                                            const GUID *guid_2,
                                            gboolean do_commit,
                                            GNCBook *book)
{
  static SCM trans_type = SCM_UNDEFINED;
  SCM result;
  SCM func;
  SCM arg;

  if (trans_scm == SCM_UNDEFINED)
    return;

  if (trans == NULL)
    return;

  g_return_if_fail (book);

  func = scm_c_eval_string("gnc:transaction-scm?");
  if (!SCM_PROCEDUREP(func))
    return;

  result = scm_call_1(func, trans_scm);
  if (!SCM_NFALSEP(result))
    return;

  func = scm_c_eval_string("gnc:transaction-scm-onto-transaction");
  if (!SCM_PROCEDUREP(func))
    return;

  if(trans_type == SCM_UNDEFINED) {
    trans_type = scm_c_eval_string("<gnc:Transaction*>");
    /* don't really need this - types are bound globally anyway. */
    if(trans_type != SCM_UNDEFINED) scm_protect_object(trans_type);
  }

  arg = gw_wcp_assimilate_ptr(trans, trans_type);

  if ((guid_1 == NULL) || (guid_2 == NULL))
  {
    SCM args = SCM_EOL;
    SCM commit;

    commit = SCM_BOOL(do_commit);

    args = scm_cons(gnc_book_to_scm (book), args);
    args = scm_cons(commit, args);
    args = scm_cons(SCM_EOL, args);
    args = scm_cons(arg, args);
    args = scm_cons(trans_scm, args);

    scm_apply(func, args, SCM_EOL);
  }
  else
  {
    SCM from, to;
    SCM map = SCM_EOL;
    SCM args = SCM_EOL;
    SCM commit;
    char *guid_str;

    args = scm_cons(gnc_book_to_scm (book), args);

    commit = SCM_BOOL(do_commit);

    args = scm_cons(commit, args);

    guid_str = guid_to_string(guid_1);
    from = scm_makfrom0str(guid_str);
    g_free (guid_str);

    guid_str = guid_to_string(guid_2);
    to = scm_makfrom0str(guid_str);
    g_free (guid_str);

    map = scm_cons(scm_cons(from, to), map);
    map = scm_cons(scm_cons(to, from), map);

    args = scm_cons(map, args);
    args = scm_cons(arg, args);
    args = scm_cons(trans_scm, args);

    scm_apply(func, args, SCM_EOL);
  }
}

/********************************************************************\
 * gnc_trans_scm_set_date                                           *
 *   set the date of a scheme transaction.                          *
 *                                                                  *
 * Args: trans_scm - the scheme transaction                         *
 *       ts        - the time to set                                *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_trans_scm_set_date(SCM trans_scm, Timespec *ts)
{
  SCM arg;

  initialize_scm_functions();

  if (!gnc_is_trans_scm(trans_scm))
    return;
  if (ts == NULL)
    return;

  arg = gnc_timespec2timepair(*ts);

  scm_call_2(setters.trans_scm_date, trans_scm, arg);
}


/********************************************************************\
 * gnc_trans_scm_set_num                                            *
 *   set the num of a scheme transaction.                           *
 *                                                                  *
 * Args: trans_scm - the scheme transaction                         *
 *       num       - the num to set                                 *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_trans_scm_set_num(SCM trans_scm, const char *num)
{
  SCM arg;

  initialize_scm_functions();

  if (!gnc_is_trans_scm(trans_scm))
    return;
  if (num == NULL)
    return;

  arg = scm_makfrom0str(num);

  scm_call_2(setters.trans_scm_num, trans_scm, arg);
}


/********************************************************************\
 * gnc_trans_scm_set_description                                    *
 *   set the description of a scheme transaction.                   *
 *                                                                  *
 * Args: trans_scm   - the scheme transaction                       *
 *       description - the description to set                       *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_trans_scm_set_description(SCM trans_scm, const char *description)
{
  SCM arg;

  initialize_scm_functions();

  if (!gnc_is_trans_scm(trans_scm))
    return;
  if (description == NULL)
    return;

  arg = scm_makfrom0str(description);

  scm_call_2(setters.trans_scm_description, trans_scm, arg);
}


/********************************************************************\
 * gnc_trans_scm_set_notes                                          *
 *   set the notes of a scheme transaction.                         *
 *                                                                  *
 * Args: trans_scm - the scheme transaction                         *
 *       notes     - the notes to set                               *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_trans_scm_set_notes(SCM trans_scm, const char *notes)
{
  SCM arg;

  initialize_scm_functions();

  if (!gnc_is_trans_scm(trans_scm))
    return;
  if (notes == NULL)
    return;

  arg = scm_makfrom0str(notes);

  scm_call_2(setters.trans_scm_notes, trans_scm, arg);
}


/********************************************************************\
 * gnc_trans_scm_append_split_scm                                   *
 *   append the scheme split onto the scheme transaction            *
 *                                                                  *
 * Args: trans_scm - the scheme transaction                         *
 *       split_scm - the scheme split to append                     *
 * Returns: Nothing                                                 *
\********************************************************************/
void
gnc_trans_scm_append_split_scm(SCM trans_scm, SCM split_scm)
{
  initialize_scm_functions();

  if (!gnc_is_trans_scm(trans_scm))
    return;
  if (!gnc_is_split_scm(split_scm))
    return;

  scm_call_2(setters.trans_scm_append_split_scm, trans_scm, split_scm);
}


/********************************************************************\
 * gnc_trans_scm_get_split_scm                                      *
 *   get the indexth scheme split of a scheme transaction.          *
 *                                                                  *
 * Args: trans_scm - the scheme transaction                         *
 *       index     - the index of the split to get                  *
 * Returns: scheme split to get, or SCM_UNDEFINED if none           *
\********************************************************************/
SCM
gnc_trans_scm_get_split_scm(SCM trans_scm, int index)
{
  SCM arg;

  initialize_scm_functions();

  if (!gnc_is_trans_scm(trans_scm))
    return SCM_UNDEFINED;

  arg = scm_int2num(index);

  return scm_call_2(getters.trans_scm_split_scm, trans_scm, arg);
}


/********************************************************************\
 * gnc_trans_scm_get_other_split_scm                                *
 *   get the other scheme split of a scheme transaction.            *
 *                                                                  *
 * Args: trans_scm - the scheme transaction                         *
 *       split_scm - the split not to get                           *
 * Returns: other scheme split, or SCM_UNDEFINED if none            *
\********************************************************************/
SCM
gnc_trans_scm_get_other_split_scm(SCM trans_scm, SCM split_scm)
{
  SCM result;

  initialize_scm_functions();

  if (!gnc_is_trans_scm(trans_scm))
    return SCM_UNDEFINED;
  if (!gnc_is_split_scm(split_scm))
    return SCM_UNDEFINED;

  result = scm_call_2(getters.trans_scm_other_split_scm, trans_scm, split_scm);

  if (!gnc_is_split_scm(result))
    return SCM_UNDEFINED;

  return result;
}


/********************************************************************\
 * gnc_trans_scm_get_num_splits                                     *
 *   get the number of scheme splits in a scheme transaction.       *
 *                                                                  *
 * Args: trans_scm - the scheme transaction                         *
 * Returns: number of scheme splits in the transaction              *
\********************************************************************/
int
gnc_trans_scm_get_num_splits(SCM trans_scm)
{
  SCM result;

  initialize_scm_functions();

  if (!gnc_is_trans_scm(trans_scm))
    return 0;

  result = scm_call_1(getters.trans_scm_split_scms, trans_scm);

  if (!SCM_LISTP(result))
    return 0;

  return SCM_LENGTH(result);
}


/********************************************************************\
 * gnc_get_debit_string                                             *
 *   return a debit string for a given account type                 *
 *                                                                  *
 * Args: account_type - type of account to get debit string for     *
 * Return: g_malloc'd debit string or NULL                          *
\********************************************************************/
char *
gnc_get_debit_string(GNCAccountType account_type)
{
  char *type_string;
  char *string;
  char *temp;
  SCM result;
  SCM arg;

  initialize_scm_functions();

  if (gnc_lookup_boolean_option("Accounts", "Use accounting labels", FALSE))
    return g_strdup(_("Debit"));

  if ((account_type < NO_TYPE) || (account_type >= NUM_ACCOUNT_TYPES))
    account_type = NO_TYPE;

  type_string = xaccAccountTypeEnumAsString(account_type);

  arg = scm_str2symbol(type_string);

  result = scm_call_1(getters.debit_string, arg);
  if (!SCM_STRINGP(result))
    return NULL;

  string = gh_scm2newstr(result, NULL);
  if (string)
  {
    temp = g_strdup (string);
    free (string);
  }
  else
    temp = NULL;

  return temp;
}


/********************************************************************\
 * gnc_get_credit_string                                            *
 *   return a credit string for a given account type                *
 *                                                                  *
 * Args: account_type - type of account to get credit string for    *
 * Return: g_malloc'd credit string or NULL                         *
\********************************************************************/
char *
gnc_get_credit_string(GNCAccountType account_type)
{
  char *type_string;
  char *string;
  char *temp;
  SCM result;
  SCM arg;

  initialize_scm_functions();

  if (gnc_lookup_boolean_option("Accounts", "Use accounting labels", FALSE))
    return g_strdup(_("Credit"));

  if ((account_type < NO_TYPE) || (account_type >= NUM_ACCOUNT_TYPES))
    account_type = NO_TYPE;

  type_string = xaccAccountTypeEnumAsString(account_type);

  arg = scm_str2symbol(type_string);

  result = scm_call_1(getters.credit_string, arg);
  if (!SCM_STRINGP(result))
    return NULL;

  string = gh_scm2newstr(result, NULL);
  if (string)
  {
    temp = g_strdup (string);
    free (string);
  }
  else
    temp = NULL;

  return temp;
}


