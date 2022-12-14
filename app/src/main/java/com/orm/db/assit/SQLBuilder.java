package com.litesuits.orm.db.assit;

import android.util.SparseArray;
import com.litesuits.orm.db.TableManager;
import com.litesuits.orm.db.annotation.*;
import com.litesuits.orm.db.enums.AssignType;
import com.litesuits.orm.db.model.*;
import com.litesuits.orm.db.model.MapInfo.MapTable;
import com.litesuits.orm.db.utils.ClassUtil;
import com.litesuits.orm.db.utils.DataUtil;
import com.litesuits.orm.db.utils.FieldUtil;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.Map.Entry;

public class SQLBuilder {

    public static final int TYPE_INSERT = 1;
    public static final int TYPE_REPLACE = 2;
    public static final int TYPE_UPDATE = 3;
    public static final String DELETE_FROM = "DELETE FROM ";
    public static final String SELECT_TABLES = "SELECT * FROM sqlite_master WHERE type='table' ORDER BY name";
    public static final String PRAGMA_TABLE_INFO = "PRAGMA table_info(";
    public static final String PARENTHESES_LEFT = "(";
    public static final String PARENTHESES_RIGHT = ")";
    public static final String IN = " IN ";
    public static final String SELECT_MAX = "SELECT MAX ";
    public static final String SELECT_ANY_FROM = "SELECT * FROM ";
    public static final String SELECT = "SELECT ";
    public static final String FROM = " FROM ";
    public static final String ORDER_BY = " ORDER BY ";
    public static final String ASC = " ASC ";
    public static final String DESC = " DESC ";
    public static final String LIMIT = " LIMIT ";
    public static final String DROP_TABLE = "DROP TABLE ";
    public static final String CREATE = "CREATE ";
    public static final String TEMP = "TEMP ";
    public static final String TABLE_IF_NOT_EXISTS = "TABLE IF NOT EXISTS ";
    public static final String PRIMARY_KEY_AUTOINCREMENT = "PRIMARY KEY AUTOINCREMENT ";
    public static final String PRIMARY_KEY = "PRIMARY KEY ";
    public static final String COMMA = ",";
    public static final String TWO_HOLDER = "(?,?)";
    public static final String BLANK = " ";
    public static final String NOT_NULL = "NOT NULL ";
    public static final String DEFAULT = "DEFAULT ";
    public static final String UNIQUE = "UNIQUE ";
    public static final String ON_CONFLICT = "ON CONFLICT ";
    public static final String CHECK = "CHECK ";
    public static final String COLLATE = "COLLATE ";
    public static final String COMMA_HOLDER = ",?";
    public static final String EQUALS_HOLDER = "=?";
    public static final String HOLDER = "?";
    public static final String INSERT = "INSERT ";
    public static final String REPLACE = "REPLACE ";
    public static final String INTO = "INTO ";
    public static final String VALUES = "VALUES";
    public static final String UPDATE = "UPDATE ";
    public static final String SET = " SET ";
    public static final String WHERE = " WHERE ";
    public static final String AND = " AND ";
    public static final String OR = " OR ";
    public static final String NOT = " NOT ";
    public static final String ASTERISK = "*";

    /**
     * ???????????????SQLite????????????sql??????
     */
    public static SQLStatement buildTableObtainAll() {
        return new SQLStatement(SELECT_TABLES, null);
    }

    /**
     * ???????????????SQLite????????????sql??????
     */
    public static SQLStatement buildColumnsObtainAll(String table) {
        return new SQLStatement(PRAGMA_TABLE_INFO + table + PARENTHESES_RIGHT, null);
    }

    /**
     * ????????????????????????????????????????????????sql??????
     */
    public static SQLStatement buildGetLastRowId(EntityTable table) {
        return new SQLStatement(SELECT_MAX + PARENTHESES_LEFT + table.key.column
                                + PARENTHESES_RIGHT + FROM + table.name, null);
    }

    /**
     * ?????????????????????sql??????
     */
    public static SQLStatement buildDropTable(EntityTable table) {
        return new SQLStatement(DROP_TABLE + table.name, null);
    }

    /**
     * ?????????????????????sql??????
     */
    public static SQLStatement buildDropTable(String tableName) {
        return new SQLStatement(DROP_TABLE + tableName, null);
    }

    /**
     * ???????????????sql??????
     *
     * create [temp] table if not exists (table-name) (co1 TEXT, co2 TEXT, UNIQUE (co1, co2))
     *
     * such as : CREATE TABLE IF NOT EXISTS table-name (_id INTEGER PRIMARY KEY AUTOINCREMENT ,xx TEXT)
     */
    public static SQLStatement buildCreateTable(EntityTable table) {
        StringBuilder sb = new StringBuilder();
        sb.append(CREATE);
        if (table.getAnnotation(Temporary.class) != null) {
            sb.append(TEMP);
        }
        sb.append(TABLE_IF_NOT_EXISTS).append(table.name).append(PARENTHESES_LEFT);
        boolean hasKey = false;
        if (table.key != null) {
            hasKey = true;
            if (table.key.assign == AssignType.AUTO_INCREMENT) {
                sb.append(table.key.column).append(DataUtil.INTEGER).append(PRIMARY_KEY_AUTOINCREMENT);
            } else {
                sb.append(table.key.column).append(DataUtil.getSQLDataType(table.key.classType)).append(PRIMARY_KEY);
            }
        }
        if (!Checker.isEmpty(table.pmap)) {
            if (hasKey) {
                sb.append(COMMA);
            }
            boolean needComma = false;
            SparseArray<ArrayList<String>> combineUniqueMap = null;
            for (Entry<String, Property> en : table.pmap.entrySet()) {
                if (needComma) {
                    sb.append(COMMA);
                } else {
                    needComma = true;
                }
                String key = en.getKey();
                sb.append(key);
                if (en.getValue() == null) {
                    sb.append(DataUtil.TEXT);
                } else {
                    Field f = en.getValue().field;
                    sb.append(DataUtil.getSQLDataType(en.getValue().classType));

                    if (f.getAnnotation(NotNull.class) != null) {
                        sb.append(NOT_NULL);
                    }
                    if (f.getAnnotation(Default.class) != null) {
                        sb.append(DEFAULT);
                        sb.append(f.getAnnotation(Default.class).value());
                        sb.append(BLANK);
                    }
                    if (f.getAnnotation(Unique.class) != null) {
                        sb.append(UNIQUE);
                    }
                    if (f.getAnnotation(Conflict.class) != null) {
                        sb.append(ON_CONFLICT);
                        sb.append(f.getAnnotation(Conflict.class).value().getSql());
                        sb.append(BLANK);
                    }

                    if (f.getAnnotation(Check.class) != null) {
                        sb.append(CHECK + PARENTHESES_LEFT);
                        sb.append(f.getAnnotation(Check.class).value());
                        sb.append(PARENTHESES_RIGHT);
                        sb.append(BLANK);
                    }
                    if (f.getAnnotation(Collate.class) != null) {
                        sb.append(COLLATE);
                        sb.append(f.getAnnotation(Collate.class).value());
                        sb.append(BLANK);
                    }
                    UniqueCombine uc = f.getAnnotation(UniqueCombine.class);
                    if (uc != null) {
                        if (combineUniqueMap == null) {
                            combineUniqueMap = new SparseArray<ArrayList<String>>();
                        }
                        ArrayList<String> list = combineUniqueMap.get(uc.value());
                        if (list == null) {
                            list = new ArrayList<String>();
                            combineUniqueMap.put(uc.value(), list);
                        }
                        list.add(key);
                    }

                }
            }
            if (combineUniqueMap != null) {
                for (int i = 0, nsize = combineUniqueMap.size(); i < nsize; i++) {
                    ArrayList<String> list = combineUniqueMap.valueAt(i);
                    if (list.size() > 1) {
                        sb.append(COMMA).append(UNIQUE).append(PARENTHESES_LEFT);
                        for (int j = 0, size = list.size(); j < size; j++) {
                            if (j != 0) {
                                sb.append(COMMA);
                            }
                            sb.append(list.get(j));
                        }
                        sb.append(PARENTHESES_RIGHT);
                    }
                }
            }
        }
        sb.append(PARENTHESES_RIGHT);
        return new SQLStatement(sb.toString(), null);
    }

    /**
     * ?????? insert ??????
     */
    public static SQLStatement buildInsertSql(Object entity, ConflictAlgorithm algorithm) {
        return buildInsertSql(entity, true, TYPE_INSERT, algorithm);
    }

    /**
     * ???????????? insert all ?????????sql????????????????????????????????????????????????
     */
    public static SQLStatement buildInsertAllSql(Object entity, ConflictAlgorithm algorithm) {
        return buildInsertSql(entity, false, TYPE_INSERT, algorithm);
    }

    /**
     * ?????? replace ??????
     */
    public static SQLStatement buildReplaceSql(Object entity) {
        return buildInsertSql(entity, true, TYPE_REPLACE, null);
    }

    /**
     * ???????????? replace all ?????????sql????????????????????????????????????????????????
     */
    public static SQLStatement buildReplaceAllSql(Object entity) {
        return buildInsertSql(entity, false, TYPE_REPLACE, null);
    }

    /**
     * ?????? insert SQL ??????
     * insert(replace) [algorithm] into {table} (key,col...) values (?,?...)
     *
     * @param entity    ??????
     * @param needValue ????????????sql?????????????????????????????????????????????
     * @param type      {@link #TYPE_INSERT}  or {@link #TYPE_REPLACE}
     * @param algorithm {@link ConflictAlgorithm}
     */
    private static SQLStatement buildInsertSql(Object entity, boolean needValue, int type,
                                               ConflictAlgorithm algorithm) {
        SQLStatement stmt = new SQLStatement();
        try {
            EntityTable table = TableManager.getTable(entity);
            StringBuilder sql = new StringBuilder(128);
            switch (type) {
                case TYPE_REPLACE:
                    sql.append(REPLACE).append(INTO);
                    break;
                case TYPE_INSERT:
                default:
                    sql.append(INSERT);
                    if (algorithm != null) {
                        sql.append(algorithm.getAlgorithm()).append(INTO);
                    } else {
                        sql.append(INTO);
                    }
                    break;
            }
            sql.append(table.name);
            sql.append(PARENTHESES_LEFT);
            sql.append(table.key.column);
            // ??????????????????SQL??????????????????for????????????SQL????????????????????????????????????????????????
            StringBuilder value = new StringBuilder();
            value.append(PARENTHESES_RIGHT).append(VALUES).append(PARENTHESES_LEFT).append(HOLDER);
            int size = 1, i = 0;
            if (!Checker.isEmpty(table.pmap)) {
                size += table.pmap.size();
            }
            Object[] args = null;
            if (needValue) {
                args = new Object[size];
                args[i++] = FieldUtil.getAssignedKeyObject(table.key, entity);
            }
            if (!Checker.isEmpty(table.pmap)) {
                for (Entry<String, Property> en : table.pmap.entrySet()) {
                    // ???????????????????????????
                    sql.append(COMMA).append(en.getKey());
                    value.append(COMMA_HOLDER);
                    // ????????????
                    if (needValue) {
                        args[i] = FieldUtil.get(en.getValue().field, entity);
                    }
                    i++;
                }
            }
            sql.append(value).append(PARENTHESES_RIGHT);
            stmt.bindArgs = args;
            stmt.sql = sql.toString();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return stmt;
    }

    /**
     * ?????? insert ??????????????????????????????
     */
    public static Object[] buildInsertSqlArgsOnly(Object entity) throws IllegalAccessException {
        EntityTable table = TableManager.getTable(entity);
        int size = 1, i = 0;
        if (!Checker.isEmpty(table.pmap)) {
            size += table.pmap.size();
        }
        Object[] args = new Object[size];
        args[i++] = FieldUtil.getAssignedKeyObject(table.key, entity);
        if (!Checker.isEmpty(table.pmap)) {
            for (Property p : table.pmap.values()) {
                // ???????????????????????????
                args[i++] = FieldUtil.get(p.field, entity);
            }
        }
        return args;
    }

    /**
     * ?????? update ??????
     */
    public static SQLStatement buildUpdateSql(Object entity, ColumnsValue cvs, ConflictAlgorithm algorithm) {
        return buildUpdateSql(entity, cvs, algorithm, true);
    }

    /**
     * ???????????? update all ?????????sql????????????????????????????????????????????????
     */
    public static SQLStatement buildUpdateAllSql(Object entity, ColumnsValue cvs, ConflictAlgorithm algorithm) {
        return buildUpdateSql(entity, cvs, algorithm, false);
    }

    /**
     * ?????? update SQL??????
     * update [algorithm] {table} set col=?,... where key=value
     *
     * @param entity    ??????
     * @param cvs       ????????????,???NULL???????????????
     * @param algorithm {@link ConflictAlgorithm}
     * @param needValue ????????????sql???????????????????????????????????????????????????????????????????????????sql???????????????????????????????????????
     */
    private static SQLStatement buildUpdateSql(Object entity, ColumnsValue cvs,
                                               ConflictAlgorithm algorithm, boolean needValue) {
        SQLStatement stmt = new SQLStatement();
        try {
            EntityTable table = TableManager.getTable(entity);
            StringBuilder sql = new StringBuilder(128);
            sql.append(UPDATE);
            if (algorithm != null) {
                sql.append(algorithm.getAlgorithm());
            }
            sql.append(table.name);
            sql.append(SET);
            // ??????????????????SQL??????????????????for????????????SQL????????????????????????????????????????????????
            int size = 1, i = 0;
            Object[] args = null;
            if (cvs != null && cvs.checkColumns()) {
                if (needValue) {
                    size += cvs.columns.length;
                    args = new Object[size];
                }
                for (; i < cvs.columns.length; i++) {
                    if (i > 0) {
                        sql.append(COMMA);
                    }
                    sql.append(cvs.columns[i]).append(EQUALS_HOLDER);
                    if (needValue) {
                        args[i] = cvs.getValue(cvs.columns[i]);
                        if (args[i] == null) {
                            args[i] = FieldUtil.get(table.pmap.get(cvs.columns[i]).field, entity);
                        }
                    }
                }
            } else if (!Checker.isEmpty(table.pmap)) {
                if (needValue) {
                    size += table.pmap.size();
                    args = new Object[size];
                }
                for (Entry<String, Property> en : table.pmap.entrySet()) {
                    if (i > 0) {
                        sql.append(COMMA);
                    }
                    sql.append(en.getKey()).append(EQUALS_HOLDER);
                    if (needValue) {
                        args[i] = FieldUtil.get(en.getValue().field, entity);
                    }
                    i++;
                }
            } else if (needValue) {
                args = new Object[size];
            }
            if (needValue) {
                args[size - 1] = FieldUtil.getAssignedKeyObject(table.key, entity);
            }
            sql.append(WHERE).append(table.key.column).append(EQUALS_HOLDER);
            stmt.sql = sql.toString();
            stmt.bindArgs = args;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return stmt;
    }

    /**
     * ?????? insert ??????????????????????????????
     */
    public static Object[] buildUpdateSqlArgsOnly(Object entity, ColumnsValue cvs) throws IllegalAccessException {
        EntityTable table = TableManager.getTable(entity);
        // ??????????????????SQL??????????????????for????????????SQL????????????????????????????????????????????????
        int size = 1, i = 0;
        Object[] args = null;
        if (cvs != null && cvs.checkColumns()) {
            size += cvs.columns.length;
            args = new Object[size];
            for (; i < cvs.columns.length; i++) {
                args[i] = cvs.getValue(cvs.columns[i]);
                if (args[i] == null) {
                    args[i] = FieldUtil.get(table.pmap.get(cvs.columns[i]).field, entity);
                }
            }
        } else if (!Checker.isEmpty(table.pmap)) {
            size += table.pmap.size();
            args = new Object[size];
            for (Entry<String, Property> en : table.pmap.entrySet()) {
                args[i] = FieldUtil.get(en.getValue().field, entity);
                i++;
            }
        } else {
            args = new Object[size];
        }
        args[size - 1] = FieldUtil.getAssignedKeyObject(table.key, entity);
        return args;
    }

    /**
     * ?????? update SQL??????
     * update [algorithm] {table} set col1=?, col2=? where ...
     *
     * @param where     ????????????
     * @param cvs       ????????????,???NULL???????????????
     * @param algorithm {@link ConflictAlgorithm}
     */
    public static SQLStatement buildUpdateSql(WhereBuilder where, ColumnsValue cvs, ConflictAlgorithm algorithm) {
        SQLStatement stmt = new SQLStatement();
        try {
            EntityTable table = TableManager.getTable(where.getTableClass());
            StringBuilder sql = new StringBuilder(128);
            sql.append(UPDATE);
            if (algorithm != null) {
                sql.append(algorithm.getAlgorithm());
            }
            sql.append(table.name);
            sql.append(SET);
            // ??????????????????SQL??????????????????for????????????SQL????????????????????????????????????????????????
            Object[] args;
            if (cvs != null && cvs.checkColumns()) {
                Object[] wArgs = where.getWhereArgs();
                if (wArgs != null) {
                    args = new Object[cvs.columns.length + wArgs.length];
                } else {
                    args = new Object[cvs.columns.length];
                }
                int i = 0;
                for (; i < cvs.columns.length; i++) {
                    if (i > 0) {
                        sql.append(COMMA);
                    }
                    sql.append(cvs.columns[i]).append(EQUALS_HOLDER);
                    args[i] = cvs.getValue(cvs.columns[i]);
                }
                if (wArgs != null) {
                    for (Object o : wArgs) {
                        args[i++] = o;
                    }
                }
            } else {
                args = where.getWhereArgs();
            }
            sql.append(where.createWhereString());
            stmt.sql = sql.toString();
            stmt.bindArgs = args;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return stmt;
    }

    /**
     * ????????????sql??????
     * delete from [table] where key=?
     */
    public static SQLStatement buildDeleteSql(Object entity) {
        SQLStatement stmt = new SQLStatement();
        try {
            EntityTable table = TableManager.getTable(entity);
            if (table.key != null) {
                stmt.sql = DELETE_FROM + table.name + WHERE + table.key.column + EQUALS_HOLDER;
                stmt.bindArgs = new String[]{String.valueOf(FieldUtil.get(table.key.field, entity))};
            } else if (!Checker.isEmpty(table.pmap)) {
                StringBuilder sb = new StringBuilder();
                sb.append(DELETE_FROM).append(table.name).append(WHERE);
                Object[] args = new Object[table.pmap.size()];
                int i = 0;
                for (Entry<String, Property> en : table.pmap.entrySet()) {
                    if (i == 0) {
                        sb.append(en.getKey()).append(EQUALS_HOLDER);
                    } else {
                        sb.append(AND).append(en.getKey()).append(EQUALS_HOLDER);
                    }
                    args[i++] = FieldUtil.get(en.getValue().field, entity);
                }
                stmt.sql = sb.toString();
                stmt.bindArgs = args;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return stmt;
    }

    /**
     * ??????????????????sql??????
     * delete from [table] where [key] in (?,?)
     *
     * ?????????collection ??????????????????999
     */
    public static SQLStatement buildDeleteSql(Collection<?> collection) {
        SQLStatement stmt = new SQLStatement();
        try {
            StringBuilder sb = new StringBuilder(256);
            EntityTable table = null;
            Object[] args = new Object[collection.size()];
            int i = 0;
            for (Object entity : collection) {
                if (i == 0) {
                    table = TableManager.getTable(entity);
                    sb.append(DELETE_FROM).append(table.name).append(WHERE)
                      .append(table.key.column).append(IN).append(PARENTHESES_LEFT).append(HOLDER);
                } else {
                    sb.append(COMMA_HOLDER);
                }
                args[i++] = FieldUtil.get(table.key.field, entity);
            }
            sb.append(PARENTHESES_RIGHT);
            stmt.sql = sb.toString();
            stmt.bindArgs = args;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return stmt;
    }

    /**
     * ??????????????????sql??????
     * delete from {table}
     */
    public static SQLStatement buildDeleteAllSql(Class<?> claxx) {
        SQLStatement stmt = new SQLStatement();
        EntityTable table = TableManager.getTable(claxx);
        stmt.sql = DELETE_FROM + table.name;
        return stmt;
    }

    /**
     * ??????????????????sql??????
     * delete form {table} where {key} in (select {key} from {table} order by {col} ASC limit {start},{end}) )
     */
    public static SQLStatement buildDeleteSql(Class<?> claxx, long start, long end, String orderAscColumn) {
        SQLStatement stmt = new SQLStatement();
        EntityTable table = TableManager.getTable(claxx);
        String key = table.key.column;
        String orderBy = Checker.isEmpty(orderAscColumn) ? key : orderAscColumn;
        StringBuilder sb = new StringBuilder();
        sb.append(DELETE_FROM).append(table.name).append(WHERE).append(key)
          .append(IN).append(PARENTHESES_LEFT)
          .append(SELECT).append(key)
          .append(FROM).append(table.name)
          .append(ORDER_BY).append(orderBy)
          .append(ASC).append(LIMIT).append(start).append(COMMA).append(end).append(PARENTHESES_RIGHT);
        stmt.sql = sb.toString();
        return stmt;
    }

    /**
     * ?????????????????????
     * alter {table} add column {col}
     */
    public static SQLStatement buildAddColumnSql(String tableName, String column) {
        SQLStatement stmt = new SQLStatement();
        stmt.sql = "ALTER TABLE " + tableName + " ADD COLUMN " + column;
        return stmt;
    }

    /**
     * ???????????????????????????
     *
     * @param tableName
     * @param column
     * @return
     */
    //public static SQLStatement buildAddPrimaryKeySql(String tableName, String column, boolean autoIncrement) {
    //    SQLStatement stmt = new SQLStatement();
    //    if (autoIncrement) {
    //        stmt.sql = "ALTER TABLE " + tableName + " ADD COLUMN " + column + " INTEGER UNIQUE AUTOINCREMENT";
    //    } else {
    //        stmt.sql = "ALTER TABLE " + tableName + " ADD COLUMN " + column + " TEXT UNIQUE";
    //    }
    //    return stmt;
    //}

    /**
     * ????????????????????????
     */
    public static MapInfo buildDelAllMappingSql(Class claxx) {
        EntityTable table1 = TableManager.getTable(claxx);
        if (!Checker.isEmpty(table1.mappingList)) {
            try {
                MapInfo mapInfo = new MapInfo();
                for (MapProperty map : table1.mappingList) {
                    EntityTable table2 = TableManager.getTable(getTypeByRelation(map));
                    // add map table info
                    String mapTableName = TableManager.getMapTableName(table1, table2);
                    MapTable mi = new MapTable(mapTableName, table1.name, table2.name);
                    mapInfo.addTable(mi);

                    // add delete mapping sql to map info
                    SQLStatement st = buildMappingDeleteAllSql(table1, table2);
                    mapInfo.addDelOldRelationSQL(st);
                }
                return mapInfo;
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return null;
    }

    /**
     * ????????????????????????
     * 1. ???????????????????????????????????????????????????????????????????????????
     * 2. ????????????????????????????????????????????????
     */
    public static MapInfo buildMappingInfo(Object entity, boolean insertNew, TableManager tableManager) {
        EntityTable table1 = TableManager.getTable(entity);
        if (!Checker.isEmpty(table1.mappingList)) {
            try {
                Object key1 = FieldUtil.get(table1.key.field, entity);
                if (key1 == null) {
                    return null;
                }
                MapInfo mapInfo = new MapInfo();
                for (MapProperty map : table1.mappingList) {
                    EntityTable table2 = TableManager.getTable(getTypeByRelation(map));
                    // add map table info
                    String mapTableName = TableManager.getMapTableName(table1, table2);
                    MapTable mi = new MapTable(mapTableName, table1.name, table2.name);
                    mapInfo.addTable(mi);
                    if (tableManager.isSQLMapTableCreated(table1.name, table2.name)) {
                        // add delete mapping sql to map info
                        SQLStatement st = buildMappingDeleteSql(key1, table1, table2);
                        mapInfo.addDelOldRelationSQL(st);
                    }

                    if (insertNew) {
                        // also insert new mapping relation
                        Object mapObject = FieldUtil.get(map.field, entity);
                        if (mapObject != null) {
                            if (map.isToMany()) {
                                ArrayList<SQLStatement> sqlList;
                                SQLStatement addSql;
                                if (mapObject instanceof Collection<?>) {
                                    sqlList = buildMappingToManySql(key1, table1, table2, (Collection<?>) mapObject);
                                    //addSql = buildMappingToManySqlFragment(key1, table1, table2,
                                    //        (Collection<?>) mapObject);
                                } else if (mapObject instanceof Object[]) {
                                    sqlList = buildMappingToManySql(key1, table1, table2,
                                            Arrays.asList((Object[]) mapObject));
                                    //addSql = buildMappingToManySqlFragment(key1, table1, table2,
                                    //        Arrays.asList((Object[]) mapObject));
                                } else {
                                    throw new RuntimeException("OneToMany and ManyToMany Relation," +
                                                               " You must use array or collection object");
                                }
                                if (Checker.isEmpty(sqlList)) {
                                    mapInfo.addNewRelationSQL(sqlList);
                                }
                                //if (addSql != null) {
                                //    mapInfo.addNewRelationSQL(addSql);
                                //}
                            } else {
                                SQLStatement st = buildMappingToOneSql(key1, table1, table2, mapObject);
                                if (st != null) {
                                    mapInfo.addNewRelationSQL(st);
                                }
                            }
                        }
                    }
                }
                return mapInfo;
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return null;
    }

    private static Class getTypeByRelation(MapProperty mp) {
        Class calxx;
        if (mp.isToMany()) {
            Class c = mp.field.getType();
            if (ClassUtil.isCollection(c)) {
                calxx = FieldUtil.getGenericType(mp.field);
            } else if (ClassUtil.isArray(c)) {
                calxx = FieldUtil.getComponentType(mp.field);
            } else {
                throw new RuntimeException(
                        "OneToMany and ManyToMany Relation, you must use collection or array object");
            }
        } else {
            calxx = mp.field.getType();
        }
        return calxx;
    }

    /**
     * ??????????????????????????????????????????
     * delete from {map table}
     */
    private static SQLStatement buildMappingDeleteAllSql(EntityTable table1,
                                                         EntityTable table2) throws IllegalArgumentException, IllegalAccessException {
        if (table2 != null) {
            String mapTableName = TableManager.getMapTableName(table1, table2);
            SQLStatement stmt = new SQLStatement();
            stmt.sql = DELETE_FROM + mapTableName;
            return stmt;
        }
        return null;
    }

    /**
     * ??????SQL???????????????Key1???????????????????????????
     * delete from {map table} where {key1=?}
     */
    public static SQLStatement buildMappingDeleteSql(Object key1, EntityTable table1,
                                                     EntityTable table2) throws IllegalArgumentException, IllegalAccessException {
        if (table2 != null) {
            String mapTableName = TableManager.getMapTableName(table1, table2);
            return buildMappingDeleteSql(mapTableName, key1, table1);
        }
        return null;
    }

    /**
     * ??????SQL???????????????Key1???????????????????????????
     * delete from {map table} where {key1=?}
     */
    public static SQLStatement buildMappingDeleteSql(String mapTableName, Object key1,
                                                     EntityTable table1) throws IllegalArgumentException, IllegalAccessException {
        if (mapTableName != null) {
            SQLStatement stmt = new SQLStatement();
            stmt.sql = DELETE_FROM + mapTableName + WHERE + table1.name + EQUALS_HOLDER;
            stmt.bindArgs = new Object[]{key1};
            return stmt;
        }
        return null;
    }

    /**
     * ??????N????????????SQL
     * replace into {table} (col1=?,col2=?) values (v1,v2),(va,vb)...
     */
    public static <T> ArrayList<SQLStatement> buildMappingToManySql(final Object key1,
                                                                    final EntityTable table1, final EntityTable table2,
                                                                    Collection<T> coll) throws Exception {
        final ArrayList<SQLStatement> sqlList = new ArrayList<SQLStatement>();
        // this will take 2 "?" holders
        CollSpliter.split(coll, SQLStatement.IN_TOP_LIMIT / 2, new CollSpliter.Spliter<T>() {
            @Override
            public int oneSplit(ArrayList<T> list) throws Exception {
                SQLStatement sql = buildMappingToManySqlFragment(key1, table1, table2, list);
                if (sql != null) {
                    sqlList.add(sql);
                }
                return 0;
            }
        });
        return sqlList;
    }

    /**
     * ??????N????????????SQL
     * replace into {table} (col1=?,col2=?) values (v1,v2),(va,vb)...
     * (?????????collection ??????)
     */
    private static SQLStatement buildMappingToManySqlFragment(Object key1, EntityTable table1,
                                                              EntityTable table2,
                                                              Collection<?> coll) throws IllegalArgumentException, IllegalAccessException {
        String mapTableName = TableManager.getMapTableName(table1, table2);
        if (!Checker.isEmpty(coll)) {
            boolean isF = true;
            StringBuilder values = new StringBuilder(128);
            ArrayList<String> list = new ArrayList<String>();
            String key1Str = String.valueOf(key1);
            for (Object o : coll) {
                Object key2 = FieldUtil.getAssignedKeyObject(table2.key, o);
                if (key2 != null) {
                    if (isF) {
                        values.append(TWO_HOLDER);
                        isF = false;
                    } else {
                        values.append(COMMA).append(TWO_HOLDER);
                    }
                    list.add(key1Str);
                    list.add(String.valueOf(key2));
                }
            }

            Object[] args = list.toArray(new String[list.size()]);
            if (!Checker.isEmpty(args)) {
                SQLStatement stmt = new SQLStatement();
                stmt.sql = REPLACE + INTO + mapTableName + PARENTHESES_LEFT + table1.name + COMMA + table2.name + PARENTHESES_RIGHT + VALUES + values;
                stmt.bindArgs = args;
                return stmt;
            }
        }
        return null;
    }

    /**
     * ??????N????????????????????????
     * insert into {table} (key1,key2) values (?,?)
     */
    public static SQLStatement buildMappingToOneSql(Object key1, EntityTable table1, EntityTable table2,
                                                    Object obj) throws IllegalArgumentException, IllegalAccessException {
        Object key2 = FieldUtil.getAssignedKeyObject(table2.key, obj);
        if (key2 != null) {
            String mapTableName = TableManager.getMapTableName(table1, table2);
            return buildMappingToOneSql(mapTableName, key1, key2, table1, table2);
        }
        return null;
    }

    /**
     * ??????N????????????????????????
     * insert into {table} (key1,key2) values (?,?)
     */
    public static SQLStatement buildMappingToOneSql(String mapTableName, Object key1, Object key2,
                                                    EntityTable table1, EntityTable table2)
            throws IllegalArgumentException, IllegalAccessException {
        if (key2 != null) {
            StringBuilder sql = new StringBuilder(128);
            sql.append(INSERT).append(INTO).append(mapTableName)
               .append(PARENTHESES_LEFT).append(table1.name)
               .append(COMMA).append(table2.name)
               .append(PARENTHESES_RIGHT).append(VALUES).append(TWO_HOLDER);
            SQLStatement stmt = new SQLStatement();
            stmt.sql = sql.toString();
            stmt.bindArgs = new Object[]{key1, key2};
            return stmt;
        }
        return null;
    }

    /**
     * ??????????????????????????????
     * select * from {map table} where {key1} in (?,?...) and {key2} in (?,?...)
     * ?????????key1List??????????????????999
     */
    public static SQLStatement buildQueryRelationSql(Class class1, Class class2, List<String> key1List) {
        return buildQueryRelationSql(class1, class2, key1List, null);
    }

    /**
     * ??????????????????????????????
     * select * from {map table} where {key1} in (?,?...) and {key2} in (?,?...)
     * ?????????keyList ??????????????????999
     */
    private static SQLStatement buildQueryRelationSql(Class class1, Class class2,
                                                      List<String> key1List, List<String> key2List) {
        final EntityTable table1 = TableManager.getTable(class1);
        final EntityTable table2 = TableManager.getTable(class2);
        QueryBuilder builder = new QueryBuilder(class1).queryMappingInfo(class2);
        ArrayList<String> keyList = new ArrayList<String>();
        StringBuilder sb = null;
        if (!Checker.isEmpty(key1List)) {
            sb = new StringBuilder();
            sb.append(table1.name).append(IN).append(PARENTHESES_LEFT);
            for (int i = 0, size = key1List.size(); i < size; i++) {
                if (i == 0) {
                    sb.append(HOLDER);
                } else {
                    sb.append(COMMA_HOLDER);
                }
            }
            sb.append(PARENTHESES_RIGHT);
            keyList.addAll(key1List);
        }
        if (!Checker.isEmpty(key2List)) {
            if (sb == null) {
                sb = new StringBuilder();
            } else {
                sb.append(AND);
            }

            sb.append(table2.name).append(IN).append(PARENTHESES_LEFT);
            for (int i = 0, size = key2List.size(); i < size; i++) {
                if (i == 0) {
                    sb.append(HOLDER);
                } else {
                    sb.append(COMMA_HOLDER);
                }
            }
            sb.append(PARENTHESES_RIGHT);
            keyList.addAll(key2List);
        }
        if (sb != null) {
            builder.where(sb.toString(), keyList.toArray(new String[keyList.size()]));
        }
        return builder.createStatement();
    }

    /**
     * ??????????????????????????????
     */
    public static SQLStatement buildQueryRelationSql(EntityTable table1, EntityTable table2, Object key1) {
        SQLStatement sqlStatement = new SQLStatement();
        sqlStatement.sql = SELECT_ANY_FROM + TableManager.getMapTableName(table1, table2)
                           + WHERE + table1.name + EQUALS_HOLDER;
        sqlStatement.bindArgs = new String[]{String.valueOf(key1)};
        return sqlStatement;
    }

    /**
     * ??????????????????????????????
     * select * from table2 where key2 = key2;
     */
    public static SQLStatement buildQueryMapEntitySql(EntityTable table2, Object key2) {
        SQLStatement sqlStatement = new SQLStatement();
        sqlStatement.sql = SELECT_ANY_FROM + table2.name + WHERE + table2.key.column + EQUALS_HOLDER;
        sqlStatement.bindArgs = new String[]{String.valueOf(key2)};
        return sqlStatement;
    }

}
