# Ownership Semantics

Ownership semantics are important whenever resources need to be cleaned up, for example, after a call to malloc or a 
call to fopen. Whenever these resources are aliased by multiple bindings, it must be clear which binding owns the 
resource and is thus responsible for cleaning the resource up when the process is finished with it. This includes 
pointer assignments to the resource, struct assignments of structs which contain pointers to the resource, argument 
passing, and return values.

# Function Results

In general, out arguments should be avoided. If a function needs to return multiple things, say, an error value and 
the actual result if no error occured, a new struct should be defined just above the function. This struct should be 
named "${FUNCTION_NAME}_result" and contain the multiple things returned. The function documentation should note which 
returned resources the caller is responsible for cleaning up.

# Variable Names

Whenever ownership semantics are important, they should be indicated by the variable name. A variable should start with 
"borrowed_" to indicate that it is being borrowed or "owned_" to indicate that it is being owned. This is most useful 
in argument names and struct member names.

If output arguments are used, they should start with "out_". The implied ownership semantics of an output argument are 
as follows:

- The pointer is borrowed from the caller's context
- The data underlying the pointer is given to the caller's context

For example, if an argument appears as `struct my_struct **out`, `out` is borrowed from the caller's context into the 
callee's context and `*out`, that is, the underlying data of type `struct my_struct *` is created by the callee and is 
given to the caller. Such a case may appear in a factory function.

Names of optional arguments, either input or output arguments, should reflect that they are optional, using the prefix 
`opt`.

# Read-Only Arguments

The underlying data type of arguments passed by pointer should be marked `const` as in 
`struct my_struct const *borrowed_my_arg`. Structs, aside from small structs with no pointer members, should never be 
passed by value.
