using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RoadBuilder : MonoBehaviour {

	public float roadWidth = 1.0f;
	public float roadHeightOffset = 0.0f;
	public float roadOffsetW = 0.0f;
	public bool doFlattenAtStart = true;
	public bool doErodeTerrain = true;
	public bool doGenerateTerrain = true;
	public bool doFlattenArroundRoad = true;
	public bool doLiftRoadToTerrain = false;

	public GameObject cone;
	public GameObject block;

	public Terrain terrain;

	public GameObject roadPrefabMesh;

	public TerrainToolkit terToolkit;

	public int iRoadTexture = 0;
	public Texture2D[] roadTextures;
	public float[] roadOffsets;
	public float[] roadWidths;

	Texture2D customRoadTexure;

	GameObject createdRoad;

	void Start()
	{
		if(terToolkit != null && doErodeTerrain)
		{
			//terToolkit.FastThermalErosion(20, 0.0f, 0.0f); //creates pits
			//terToolkit.FastHydraulicErosion(100, 1.0f, 0.0f); //creates washouts
			//terToolkit.FullHydraulicErosion(1, 10.0f, 1.0f, .3f, 2.0f);
			terToolkit.SmoothTerrain(10, 1.0f);
		}
	}

	public void DestroyRoad()
	{
		GameObject[] prev = GameObject.FindGameObjectsWithTag("road_mesh");

		foreach(GameObject g in prev)
			Destroy(g);

		//advance road index into texture list.
		iRoadTexture += 1;
	}

	public void SetNewRoadVariation(int iVariation)
	{
		if(roadTextures.Length > 0)		
			customRoadTexure = roadTextures[ iVariation % roadTextures.Length ];

		if(roadOffsets.Length > 0)
			roadOffsetW = roadOffsets[ iVariation % roadOffsets.Length ];

		if(roadWidths.Length > 0)
			roadWidth = roadWidths[ iVariation % roadWidths.Length ];
		
	}

	public void NegateYTiling()
	{
		//todo
		if(createdRoad == null)
			return;
		
		MeshRenderer mr = createdRoad.GetComponent<MeshRenderer>();
		Vector2 ms = mr.material.mainTextureScale;
		ms.y *= -1.0f;
		mr.material.mainTextureScale = ms;
	}

	public void InitRoad(CarPath path)
	{
		if(terToolkit != null && doFlattenAtStart)
		{
			terToolkit.Flatten();
		}

		if(terToolkit != null && doGenerateTerrain)
		{
			terToolkit.PerlinGenerator(1, 0.1f, 10, 0.5f);
			//terToolkit.NormaliseTerrain(0.0f, 0.001f, 0.5f);
		}
		
		GameObject go = GameObject.Instantiate(roadPrefabMesh);
		MeshRenderer mr = go.GetComponent<MeshRenderer>();
		MeshFilter mf = go.GetComponent<MeshFilter>();
		Mesh mesh = new Mesh();
		mf.mesh = mesh;
		createdRoad = go;

		if(customRoadTexure != null)
		{
			mr.material.mainTexture = customRoadTexure;
		}
		else if(roadTextures != null && iRoadTexture < roadTextures.Length)
		{
			Texture2D t = roadTextures[iRoadTexture];

			if(mr != null && t != null)
			{
				mr.material.mainTexture = t;
			}
		}

		go.tag = "road_mesh";

		int numQuads = path.nodes.Count - 1; // two points can decide one quad, each quad is composed by two triangles, two quads needs 3*2=6 vertices, vertices num is the same as uv's
		int numVerts = (numQuads + 1) * 2;
		int numTris = numQuads * 2;

		Vector3[] vertices = new Vector3[numVerts];

		int numTriIndecies = numTris * 3;
		int[] tri = new int[numTriIndecies];

		int numNormals = numVerts;
		Vector3[] normals = new Vector3[numNormals];

		int numUvs = numVerts;
		Vector2[] uv = new Vector2[numUvs];

		for(int iN = 0; iN < numNormals; iN++)
			normals[iN] = Vector3.up;

		int iNode = 0;

		Vector3 posA = Vector3.zero; //first point
		Vector3 posB = Vector3.zero; //second point

		Vector3 vLength = Vector3.one; // the road length between two points
		Vector3 vWidth = Vector3.one; // the road width

		for(int iVert = 0; iVert < numVerts; iVert += 2)
		{
			PathNode nodeA;
			PathNode nodeB;
			if(iNode + 1 < path.nodes.Count)
			{
				nodeA = path.nodes[iNode];
				nodeB = path.nodes[iNode + 1];
				posA = nodeA.pos;
				posB = nodeB.pos;

				vLength = posB - posA;
				vWidth = Vector3.Cross(vLength, Vector3.up); // width's half value equals the length

				if(terToolkit != null && doFlattenArroundRoad  && (iVert % 10) == 0)
				{
					terToolkit.FlattenArround(posA + vWidth.normalized * roadOffsetW, 10.0f, 30.0f);
				}

				if(doLiftRoadToTerrain)
				{
					posA.y = terrain.SampleHeight(posA) + 1.0f;
				}

				posA.y += roadHeightOffset;
			}
			else
			{
				nodeA = path.nodes[iNode];
				posA = nodeA.pos;
				posA.y += roadHeightOffset;
			}

			Vector3 placeTmp = posA + vWidth.normalized * nodeA.thing_offset;
			placeTmp.y = placeTmp.y + 0.3f;
			if(nodeA.thing == "cone"){
				GameObject coneO=Instantiate(cone, placeTmp, nodeA.thing_rot) as GameObject;
				coneO.AddComponent<Rigidbody>();
				coneO.AddComponent<BoxCollider>();
				coneO.tag = "pathNode";
			}else if(nodeA.thing == "block"){
				GameObject blockO=Instantiate(block, placeTmp, nodeA.thing_rot) as GameObject;
				blockO.AddComponent<Rigidbody>();
				blockO.AddComponent<BoxCollider>();
				blockO.tag = "pathNode";
			}

			Vector3 leftPos = posA + vWidth.normalized * roadWidth + vWidth.normalized * roadOffsetW; // left side in the road of first point, roadOffsetW means how far from the middle
			Vector3 rightPos = posA - vWidth.normalized * roadWidth + vWidth.normalized * roadOffsetW; // right side in the road of first point

			vertices[iVert] = leftPos;
			vertices[iVert + 1] = rightPos;

			uv[iVert] = new Vector2(0.2f * iNode, 0.0f);
			uv[iVert + 1] = new Vector2(0.2f * iNode, 1.0f);

			iNode++;
		}

		int iVertOffset = 0;
		int iTriOffset = 0;

		for(int iQuad = 0; iQuad < numQuads; iQuad++)// the triangle points are left and right points of the road, but the texture is related to quads
		{
			tri[0 + iTriOffset] = 0 + iVertOffset;
			tri[1 + iTriOffset] = 2 + iVertOffset;
			tri[2 + iTriOffset] = 1 + iVertOffset;

			tri[3 + iTriOffset] = 2 + iVertOffset;
			tri[4 + iTriOffset] = 3 + iVertOffset;
			tri[5 + iTriOffset] = 1 + iVertOffset;

			iVertOffset += 2;
			iTriOffset += 6;
		}


		mesh.vertices = vertices;
		mesh.triangles = tri;
		mesh.normals = normals;
		mesh.uv = uv;

		mesh.RecalculateBounds();



		if(terToolkit != null && doErodeTerrain)
		{
			//terToolkit.FastThermalErosion(20, 0.0f, 0.0f); //creates pits
			//terToolkit.FastHydraulicErosion(100, 1.0f, 0.0f); //creates washouts
			//terToolkit.FullHydraulicErosion(1, 10.0f, 1.0f, .3f, 2.0f);
			terToolkit.SmoothTerrain(10, 1.0f);

			if(doFlattenArroundRoad)
			{
				foreach(PathNode n in path.nodes)
				{
					terToolkit.FlattenArround(n.pos, 8.0f, 10.0f);
				}
			}

			float[] slopeStops = new float[2];
			float[] heightStops = new float[2];

			slopeStops[0] = 1.0f;
			slopeStops[1] = 2.0f;

			heightStops[0] = 4.0f;
			heightStops[1] = 10.0f;

			//terToolkit.TextureTerrain(slopeStops, heightStops, textures);
		}
	}
}
