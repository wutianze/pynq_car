using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Text;
using System.IO;

public class TrackParams
{
    public enum State
    {
        Straight,
        CurveX,
        CurveY,
        CurveZ,
        SpeedLimit,
        AngleDX, //when changing the dx curve setting
        AngleDY, //when changing the dy curve setting
        AngleDZ, //when changing the dz curve setting
        ForkA,   //when laying a track element that splits in two.
        ForkB,   //when starting the second line that splits from the fork
        MergeA,  //when we should look for the nearest track to merge from a fork
        MergeB,  //when we should look for the nearest track to merge from a fork
        End,     //terminate current line
        CONE,    // the cone in road
        BLOCK
    }

    public State state;
    public int numToSet;
    public Quaternion rotCur;
    public Quaternion dRot;
    public Vector3 lastPos;
}

[System.Serializable]
public class TrackScriptElem
{
    public TrackParams.State state;
    public int numToSet;
    public float value;
    public float value2;



    public TrackScriptElem(TrackParams.State s = TrackParams.State.Straight, float si = 1.0f, float si2 = 1.0f, int num = 1)
    {
        state = s;
        numToSet = num;
        value = si;
        value2 = si2;
    }
}

public class TrackScript
{
    public List<TrackScriptElem> track;

    public void Build(TrackScriptElem el)
    {
        if(track.Count == 0)
        {
            track.Add(el);
        }
        else
        {
            TrackScriptElem lastElem = track[track.Count - 1];

            if(lastElem.state == el.state && lastElem.value == el.value)
            {
                lastElem.numToSet += 1;
            }
            else
            {
                track.Add(el);
            }
        }
    }

    public bool Write(string filename)
    {
        StringBuilder sb = new StringBuilder();

        System.IO.File.WriteAllText(filename, sb.ToString());

        return true;
    }

    public bool Read(string filename)
    {
        track = new List<TrackScriptElem>();

        Debug.Log("loading: " + filename);

        /*
        TextAsset bindata = Resources.Load(filename) as TextAsset;

		if(bindata == null){
            Debug.LogError("loading fail");
			return false;
        }
        else{
            Debug.Log("file content: " + bindata.text);
        }
        string[] lines = bindata.text.Split('\n');
        */
        string[] lines = File.ReadAllLines(filename);

        foreach(string line in lines)
        {
            Debug.Log("reading the line: " + line);
            string[] tokens = line.Split(' ');

            string command = tokens[0];
            /*string args = "0";
            if (tokens.Length >= 2){
                args = tokens[1];
            }*/

            if (command.StartsWith("//"))
                continue;

            TrackScriptElem tse = new TrackScriptElem();

            /*else if (command == "U") 
            {
                tse.state = TrackParams.State.CurveZ;
                tse.value = 1f;
                tse.numToSet = int.Parse(args);
            }
            else if (command == "D")
            {
                tse.state = TrackParams.State.CurveZ;
                tse.value = -1f;
                tse.numToSet = int.Parse(args);
            }
            else if (command == "RL")
            {
                tse.state = TrackParams.State.CurveX;
                tse.value = 1f;
                tse.numToSet = int.Parse(args);
            }
            else if (command == "RR")
            {
                tse.state = TrackParams.State.CurveX;
                tse.value = -1f;
                tse.numToSet = int.Parse(args);
            }
            else if (command == "SPEED_LIMIT")
            {
                tse.state = TrackParams.State.SpeedLimit;
                tse.value = float.Parse(args);
                tse.numToSet = 0;
            }
            else if (command == "DX")
            {
                tse.state = TrackParams.State.AngleDX;
                tse.value = float.Parse(args);
                tse.numToSet = 0;
            }
            else if (command == "DZ")
            {
                tse.state = TrackParams.State.AngleDZ;
                tse.value = float.Parse(args);
                tse.numToSet = 0;
            }
            else if (command == "FORK_A")
            {
                tse.state = TrackParams.State.ForkA;
                tse.value = float.Parse(args);
                tse.numToSet = 0;
            }
            else if (command == "FORK_B")
            {
                tse.state = TrackParams.State.ForkB;
                tse.value = float.Parse(args);
                tse.numToSet = 0;
            }
            else if (command == "MERGE_A")
            {
                tse.state = TrackParams.State.MergeA;
                tse.value = float.Parse(args);
                tse.numToSet = 0;
            }
            else if (command == "MERGE_B")
            {
                tse.state = TrackParams.State.MergeB;
                tse.value = float.Parse(args);
                tse.numToSet = 0;
            }
            else if (command == "END")
            {
                tse.state = TrackParams.State.End;
                tse.value = float.Parse(args);
                tse.numToSet = 0;
            }
            */
            if(command == "S")// straight line, value is the length
            {
                tse.state = TrackParams.State.Straight;
                tse.value = 1f;
                tse.numToSet = int.Parse(tokens[1]);
            }
            else if (command == "L")// turn left, value is the length
            {
                tse.state = TrackParams.State.CurveY;
                tse.value = -1f;
                tse.numToSet = int.Parse(tokens[1]);
            }
            else if (command == "R")// turn right,value is the length
            {
                tse.state = TrackParams.State.CurveY;
                tse.value = 1f;
                tse.numToSet = int.Parse(tokens[1]);
            }
            else if (command == "DY")// set the turn angle
            {
                tse.state = TrackParams.State.AngleDY;
                tse.value = float.Parse(tokens[1]);
                tse.numToSet = 0;
            }
            else if(command == "CONE")// value is how far from the start middle(road point, not the road middle), every thing will take one road length
            {
                tse.state = TrackParams.State.CONE;
                tse.value = float.Parse(tokens[1]);
                tse.value2 = float.Parse(tokens[2]);
                tse.numToSet = 1;
            }
            else if(command == "BLOCK")
            {
                tse.state = TrackParams.State.BLOCK;
                tse.value = float.Parse(tokens[1]);
                tse.value2 = float.Parse(tokens[2]);
                tse.numToSet = 1;
            }
            else
            {
                Debug.Log("unknown command: " + command);
                continue;
            }

            track.Add(tse);
        }

		return track.Count > 0;
    }
}