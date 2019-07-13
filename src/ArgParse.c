/***************************************************************************************************

  Zyan Core Library (Zycore-C)

  Original Author : Joel Hoener

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.

***************************************************************************************************/

#include <Zycore/ArgParse.h>
#include <Zycore/LibC.h>

/* ============================================================================================== */
/* Exported functions                                                                             */
/* ============================================================================================== */

ZyanStatus ZyanArgParse(const ZyanArgParseConfig *cfg, ZyanVector* parsed)
{
    ZYAN_ASSERT(cfg);
    ZYAN_ASSERT(parsed);

    if (cfg->min_unnamed_args > cfg->max_unnamed_args)
    {
        return ZYAN_STATUS_INVALID_ARGUMENT;
    }

    // Check argument syntax.
    for (const ZyanArgParseDefinition* arg = cfg->args; arg && arg->name; ++arg)
    {
        // TODO: Duplicate check

        if (!arg->name)
        {
            return ZYAN_STATUS_INVALID_ARGUMENT;
        }

        ZyanUSize arg_len = ZYAN_STRLEN(arg->name);
        if (arg_len < 2 || arg->name[0] != '-')
        {
            return ZYAN_STATUS_INVALID_ARGUMENT;
        }

        // Single dash arguments only accept a single char name.
        if (arg->name[1] != '-' && arg_len != 2)
        {
            return ZYAN_STATUS_INVALID_ARGUMENT;
        }
    }

    // Initialize output vector.
    ZYAN_CHECK(ZyanVectorInit(parsed, sizeof(ZyanArgParseArg), cfg->argc, ZYAN_NULL));

    ZyanStatus err;
    ZyanBool accept_dash_args = ZYAN_TRUE;
    ZyanUSize num_unnamed_args = 0;
    for (ZyanUSize i = 1; i < cfg->argc; ++i)
    {
        const char* cur_arg = cfg->argv[i];
        ZyanUSize arg_len = ZYAN_STRLEN(cfg->argv[i]);

        // Double-dash argument?
        if (accept_dash_args && arg_len >= 2 && ZYAN_MEMCMP(cur_arg, "--", 2) == 0)
        {
            // GNU style end of argument parsing.
            if (arg_len == 2)
            {
                accept_dash_args = ZYAN_FALSE;
            }
            // Regular double-dash argument.
            else
            {
                // Allocate parsed argument struct.
                ZyanArgParseArg* parsed_arg;
                ZYAN_CHECK(ZyanVectorEmplace(parsed, (void**)&parsed_arg, ZYAN_NULL));
                ZYAN_MEMSET(parsed_arg, 0, sizeof(*parsed_arg));

                // Find corresponding argument definition.
                for (const ZyanArgParseDefinition* arg = cfg->args; arg && arg->name; ++arg)
                {
                    if (ZYAN_STRCMP(arg->name, cur_arg) == 0)
                    {
                        parsed_arg->arg = arg;
                        break;
                    }
                }

                // Search exhausted & argument not found. RIP.
                if (!parsed_arg->arg)
                {
                    err = ZYAN_ARGPARSE_STATUS_ARG_NOT_UNDERSTOOD;
                    goto failure;
                }

                // Does the argument expect a value? If yes, consume next token.
                if (!parsed_arg->arg->boolean)
                {
                    if (i == cfg->argc - 1)
                    {
                        err = ZYAN_ARGPARSE_STATUS_ARG_MISSES_VALUE;
                        goto failure;
                    }
                    parsed_arg->has_value = ZYAN_TRUE;
                    ZYAN_CHECK(ZyanStringViewInsideBuffer(&parsed_arg->value, cfg->argv[++i]));
                }
            }

            // Continue parsing at next token.
            continue;
        }

        // Single-dash argument?
        // TODO: How to deal with just dashes? Current code treats it as unnamed arg.
        if (accept_dash_args && arg_len > 1 && cur_arg[0] == '-')
        {
            // Iterate argument token chars until there are either no more chars left
            // or we encounter a non-boolean argument, in which case we consume the
            // remaining chars as its value.
            for (const char* read_ptr = cur_arg + 1; *read_ptr; ++read_ptr)
            {
                // Allocate parsed argument struct.
                ZyanArgParseArg* parsed_arg;
                ZYAN_CHECK(ZyanVectorEmplace(parsed, (void**)&parsed_arg, ZYAN_NULL));
                ZYAN_MEMSET(parsed_arg, 0, sizeof(*parsed_arg));

                // Find corresponding argument definition.
                for (const ZyanArgParseDefinition* arg = cfg->args; arg && arg->name; ++arg)
                {
                    if (ZYAN_STRLEN(arg->name) == 2 &&
                        arg->name[0] == '-' &&
                        arg->name[1] == *read_ptr
                    ) {
                        parsed_arg->arg = arg;
                        break;
                    }
                }

                // Search exhausted, no match found?
                if (!parsed_arg->arg)
                {
                    err = ZYAN_ARGPARSE_STATUS_ARG_NOT_UNDERSTOOD;
                    goto failure;
                }

                // Requires value?
                if (!parsed_arg->arg->boolean)
                {
                    // If there are chars left, consume them (e.g. `-n1000`).
                    if (read_ptr[1])
                    {
                        parsed_arg->has_value = ZYAN_TRUE;
                        ZYAN_CHECK(ZyanStringViewInsideBuffer(&parsed_arg->value, read_ptr + 1));
                    }
                    // If not, consume next token (e.g. `-n 1000`).
                    else
                    {
                        if (i == cfg->argc - 1)
                        {
                            err = ZYAN_ARGPARSE_STATUS_ARG_MISSES_VALUE;
                            goto failure;
                        }
                        
                        parsed_arg->has_value = ZYAN_TRUE;
                        ZYAN_CHECK(ZyanStringViewInsideBuffer(&parsed_arg->value, cfg->argv[++i]));
                    }

                    // Either way, continue with next argument.
                    goto continue_main_loop;
                }
            }
        }

        // Still here? We're looking at an unnamed argument.
        ++num_unnamed_args;
        if (num_unnamed_args > cfg->max_unnamed_args)
        {
            err = ZYAN_ARGPARSE_STATUS_TOO_MANY_ARGS;
            goto failure;
        }

        // Allocate parsed argument struct.
        ZyanArgParseArg* parsed_arg;
        ZYAN_CHECK(ZyanVectorEmplace(parsed, (void**)&parsed_arg, ZYAN_NULL));
        ZYAN_MEMSET(parsed_arg, 0, sizeof(*parsed_arg));
        parsed_arg->has_value = ZYAN_TRUE;
        ZYAN_CHECK(ZyanStringViewInsideBuffer(&parsed_arg->value, cur_arg));

    continue_main_loop:;
    }

    // All tokens processed. Do we have enough unnamed arguments?
    if (num_unnamed_args < cfg->min_unnamed_args)
    {
        err = ZYAN_ARGPARSE_STATUS_TOO_FEW_ARGS;
        goto failure;
    }

    // Yay!
    return ZYAN_STATUS_SUCCESS;

failure:
    ZYAN_CHECK(ZyanVectorDestroy(parsed));
    return err;
}

/* ============================================================================================== */
