# Postgres dataclips extension

**Status: development stalled; intent to resume**

This is not yet functional, but the idea would be to be able to query
existing [dataclips](https://devcenter.heroku.com/articles/dataclips)
via a function and via FDW.

```sql
select dt.* 
from dataclip("gqhkylscctbsgrohaythgdmlfcnn") 
  as dt(foo text, bar json);
where dt.deleted_at is null;
```
