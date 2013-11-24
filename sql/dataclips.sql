-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION dataclip" to load this file. \quit

CREATE FUNCTION dataclip(id text)
RETURNS setof record
AS 'MODULE_PATHNAME','dataclip'
LANGUAGE C STABLE STRICT;