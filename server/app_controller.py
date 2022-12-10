from flask import Flask, request, json, jsonify
import pymysql
import sqlite3
import time

app = Flask(__name__)

def insert(fire_detected, polution):
    now = time.localtime()
    sql = "insert into nodam values(?, ?, ?)"
    vals = (time.strftime("%Y%m%d %H:%M:%S", now), fire_detected, polution)
    db.execute(sql, vals)
    conn.commit()

@app.route("/post_data", methods=["POST"])
def test():
    params = request.get_json()
    res = False
    print(params)
    json_obj = json.dumps(params)
    json_obj = json.loads(json_obj)
    for key, val in json_obj.items():
        print("key=" + key)
        print(val)
        if(key == "fire"):
            fire_detected = int(val)
        elif(key == "polution"):
            polution = int(val)
    #fire_detected = int(json_obj['fire'])
    #polution = int(json_obj['polution'])
    insert(fire_detected, polution)
    response = {
            "result" : "ok"
            }
    return jsonify(response)

if __name__ == "__main__":
    conn = sqlite3.connect('test.db', check_same_thread=False)
    db = conn.cursor()
    db.execute("SELECT * from sqlite_master WHERE type=\"table\" AND name=\"nodam\"")
    rows = db.fetchall()
    if not rows:
        db.execute("CREATE TABLE nodam(date text, fire integer, polution integer)")
        db.execute("INSERT INTO nodam VALUES(20221208, 1, 1)")
        conn.commit()
    app.run(debug = True, host='0.0.0.0', port=20000)
    conn.close()
