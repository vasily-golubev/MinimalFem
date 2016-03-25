/*MinimalFEM

Author: Stanislav Pidhorskyi (Podgorskiy)
stanislav@podgorskiy.com
stpidhorskyi@mix.wvu.edu

The source code available here: https://github.com/podgorskiy/MinimalFEM/

The MIT License (MIT)

Copyright (c) 2015 Stanislav Pidhorskyi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#if defined (RECT) && defined (ANALYTICAL)
#include <ginac/ginac.h>
#endif

struct Element
{
	void CalculateStiffnessMatrix(const Eigen::Matrix3f& D, std::vector<Eigen::Triplet<float> >& triplets);

#ifdef RECT
	Eigen::Matrix<float, 3, 8> B;
	int nodesIds[4];
#else
	Eigen::Matrix<float, 3, 6> B;
	int nodesIds[3];
#endif
};

struct Constraint
{
	enum Type
	{
		UX = 1 << 0,
		UY = 1 << 1,
		UXY = UX | UY
	};
	int node;
	Type type;
};

int                      	nodesCount;
Eigen::VectorXf          	nodesX;
Eigen::VectorXf          	nodesY;
Eigen::VectorXf          	loads;
std::vector< Element >   	elements;
std::vector< Constraint >	constraints;

Eigen::Matrix3f D;

#if defined (RECT) && defined (ANALYTICAL)
GiNaC::matrix g_D(3, 3);
GiNaC::matrix g_K(8,8);
GiNaC::symbol x("x"), y("y");
GiNaC::symbol x_0("x0"), y_0("y0");
GiNaC::symbol x_1("x1"), y_3("y3");
GiNaC::symbol IC_0i[4] = {GiNaC::symbol("ic_00"),
				GiNaC::symbol("ic_01"),
				GiNaC::symbol("ic_02"),
				GiNaC::symbol("ic_03")};
GiNaC::symbol IC_1i[4] = {GiNaC::symbol("ic_10"),
				GiNaC::symbol("ic_11"),
				GiNaC::symbol("ic_12"),
				GiNaC::symbol("ic_13")};
GiNaC::symbol IC_2i[4] = {GiNaC::symbol("ic_20"),
				GiNaC::symbol("ic_21"),
				GiNaC::symbol("ic_22"),
				GiNaC::symbol("ic_23")};
#endif

#if defined (RECT) && defined (ANALYTICAL)
void PrecalculateIntegrals() {
	GiNaC::matrix g_B(3, 8);
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 8; j++)
			g_B(i, j) = 0.0f;
	for (int i = 0; i < 4; i++) {
		g_B(0, 2 * i  + 0) = IC_0i[i] + y * IC_1i[i];
		g_B(0, 2 * i + 1) = 0.0;
		g_B(1, 2 * i + 0) = 0.0;
		g_B(1, 2 * i + 1) = IC_2i[i] + x * IC_1i[i];
		g_B(2, 2 * i + 0) = IC_2i[i] + x * IC_1i[i];
		g_B(2, 2 * i + 1) = IC_0i[i] + y * IC_1i[i];
	}

	g_K = (g_B.transpose()).mul(g_D);
	g_K = g_K.mul(g_B);
	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 8; j++) {
			GiNaC::integral ex = GiNaC::integral(x, x_0, x_1, g_K(i, j));
			ex = GiNaC::integral(y, y_0, y_3, ex.eval_integ());
			g_K(i, j) = ex.eval_integ();
		}
}
#endif

#if defined (RECT) && defined (ANALYTICAL)
void IntegrateRect(Eigen::Matrix4f &IC, Eigen::Vector4f &vx, Eigen::Vector4f &vy, std::vector<Eigen::Triplet<float> >& triplets, int nodesIds[])
{
	GiNaC::lst data(IC_0i[0] == IC(0, 0),
			IC_0i[1] == IC(0, 1),
			IC_0i[2] == IC(0, 2),
			IC_0i[3] == IC(0, 3),
			IC_1i[0] == IC(1, 0),
			IC_1i[1] == IC(1, 1),
			IC_1i[2] == IC(1, 2),
			IC_1i[3] == IC(1, 3),
			IC_2i[0] == IC(2, 0),
			IC_2i[1] == IC(2, 1),
			IC_2i[2] == IC(2, 2),
			IC_2i[3] == IC(2, 3),
			x_0 == vx(0),
			x_1 == vx(1),
			y_0 == vy(0),
			y_3 == vy(3)
			);

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			// FIXME If we need to add smth like trplt13, etc?
			Eigen::Triplet<float> trplt11(2 * nodesIds[i] + 0, 2 * nodesIds[j] + 0, GiNaC::ex_to<GiNaC::numeric>((g_K(2 * i + 0, 2 * j + 0).subs(data))).to_double());
			Eigen::Triplet<float> trplt12(2 * nodesIds[i] + 0, 2 * nodesIds[j] + 1, GiNaC::ex_to<GiNaC::numeric>((g_K(2 * i + 0, 2 * j + 1).subs(data))).to_double());
			Eigen::Triplet<float> trplt21(2 * nodesIds[i] + 1, 2 * nodesIds[j] + 0, GiNaC::ex_to<GiNaC::numeric>((g_K(2 * i + 1, 2 * j + 0).subs(data))).to_double());
			Eigen::Triplet<float> trplt22(2 * nodesIds[i] + 1, 2 * nodesIds[j] + 1, GiNaC::ex_to<GiNaC::numeric>((g_K(2 * i + 1, 2 * j + 1).subs(data))).to_double());

			triplets.push_back(trplt11);
			triplets.push_back(trplt12);
			triplets.push_back(trplt21);
			triplets.push_back(trplt22);
		}
	}
}
#endif

void Element::CalculateStiffnessMatrix(const Eigen::Matrix3f& D, std::vector<Eigen::Triplet<float> >& triplets)
{
#ifdef RECT
	Eigen::Vector4f x, y;
	x << nodesX[nodesIds[0]], nodesX[nodesIds[1]], nodesX[nodesIds[2]], nodesX[nodesIds[3]];
	y << nodesY[nodesIds[0]], nodesY[nodesIds[1]], nodesY[nodesIds[2]], nodesY[nodesIds[3]];

	Eigen::Matrix4f C;
	C << x, Eigen::Vector4f(x(0) * y(0), x(1) * y(1), x(2) * y(2), x(3) * y(3)), y, Eigen::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);

	Eigen::Matrix4f IC = C.inverse();
#ifdef ANALYTICAL
	IntegrateRect(IC, x, y, triplets, nodesIds);
#else
	// FIXME Integral with 4 points using wikipedia
	/* Order of vertex in rectangle
	  3-------------2
	  |		|
	  |		|
	  0-------------1
	*/
	Eigen::Matrix<float, 8, 8> K;
	K.setZero();
	for (int ind = 0; ind < 4; ind++) {
		float coordX = x(ind);
		float coordY = y(ind);
		for (int i = 0; i < 4; i++)
		{
			B(0, 2 * i + 0) = IC(0, i) + coordY * IC(1, i);
			B(0, 2 * i + 1) = 0.0f;
			B(1, 2 * i + 0) = 0.0f;
			B(1, 2 * i + 1) = IC(2, i) + coordX * IC(1, i);
			B(2, 2 * i + 0) = IC(2, i) + coordX * IC(1, i);
			B(2, 2 * i + 1) = IC(0, i) + coordY * IC(1, i);
		}
		K += B.transpose() * D * B;
	}
	K = 0.25f * 0.25f * (x(1) - x(0)) * (y(2) - y(1)) * K;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			// FIXME If we need to add smth like trplt13, etc?
			Eigen::Triplet<float> trplt11(2 * nodesIds[i] + 0, 2 * nodesIds[j] + 0, K(2 * i + 0, 2 * j + 0));
			Eigen::Triplet<float> trplt12(2 * nodesIds[i] + 0, 2 * nodesIds[j] + 1, K(2 * i + 0, 2 * j + 1));
			Eigen::Triplet<float> trplt21(2 * nodesIds[i] + 1, 2 * nodesIds[j] + 0, K(2 * i + 1, 2 * j + 0));
			Eigen::Triplet<float> trplt22(2 * nodesIds[i] + 1, 2 * nodesIds[j] + 1, K(2 * i + 1, 2 * j + 1));

			triplets.push_back(trplt11);
			triplets.push_back(trplt12);
			triplets.push_back(trplt21);
			triplets.push_back(trplt22);
		}
	}
#endif
#else
	Eigen::Vector3f x, y;
	x << nodesX[nodesIds[0]], nodesX[nodesIds[1]], nodesX[nodesIds[2]];
	y << nodesY[nodesIds[0]], nodesY[nodesIds[1]], nodesY[nodesIds[2]];
	
	Eigen::Matrix3f C;
	C << Eigen::Vector3f(1.0f, 1.0f, 1.0f), x, y;
	
	Eigen::Matrix3f IC = C.inverse();

	for (int i = 0; i < 3; i++)
	{
		B(0, 2 * i + 0) = IC(1, i);
		B(0, 2 * i + 1) = 0.0f;
		B(1, 2 * i + 0) = 0.0f;
		B(1, 2 * i + 1) = IC(2, i);
		B(2, 2 * i + 0) = IC(2, i);
		B(2, 2 * i + 1) = IC(1, i);
	}
	Eigen::Matrix<float, 6, 6> K = B.transpose() * D * B * C.determinant() / 2.0f;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			Eigen::Triplet<float> trplt11(2 * nodesIds[i] + 0, 2 * nodesIds[j] + 0, K(2 * i + 0, 2 * j + 0));
			Eigen::Triplet<float> trplt12(2 * nodesIds[i] + 0, 2 * nodesIds[j] + 1, K(2 * i + 0, 2 * j + 1));
			Eigen::Triplet<float> trplt21(2 * nodesIds[i] + 1, 2 * nodesIds[j] + 0, K(2 * i + 1, 2 * j + 0));
			Eigen::Triplet<float> trplt22(2 * nodesIds[i] + 1, 2 * nodesIds[j] + 1, K(2 * i + 1, 2 * j + 1));

			triplets.push_back(trplt11);
			triplets.push_back(trplt12);
			triplets.push_back(trplt21);
			triplets.push_back(trplt22);
		}
	}
#endif
}

void SetConstraints(Eigen::SparseMatrix<float>::InnerIterator& it, int index)
{
	if (it.row() == index || it.col() == index)
	{
		it.valueRef() = it.row() == it.col() ? 1.0f : 0.0f;
	}
}

void ApplyConstraints(Eigen::SparseMatrix<float>& K, const std::vector<Constraint>& constraints)
{
	std::vector<int> indicesToConstraint;

	for (std::vector<Constraint>::const_iterator it = constraints.begin(); it != constraints.end(); ++it)
	{
		if (it->type & Constraint::UX)
		{
			indicesToConstraint.push_back(2 * it->node + 0);
		}
		if (it->type & Constraint::UY)
		{
			indicesToConstraint.push_back(2 * it->node + 1);
		}
	}

	for (int k = 0; k < K.outerSize(); ++k)
	{
		for (Eigen::SparseMatrix<float>::InnerIterator it(K, k); it; ++it)
		{
			for (std::vector<int>::iterator idit = indicesToConstraint.begin(); idit != indicesToConstraint.end(); ++idit)
			{
				SetConstraints(it, *idit);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	if ( argc != 3 )
    {
        std::cout<<"usage: "<< argv[0] <<" <input file> <output file>\n";
        return 1;
    }
	
	std::ifstream infile(argv[1]);
	std::ofstream outfile(argv[2]);
	
	float poissonRatio, youngModulus;
	infile >> poissonRatio >> youngModulus;

	D <<
		1.0f,        	poissonRatio,	0.0f,
		poissonRatio,	1.0,         	0.0f,
		0.0f,        	0.0f,        	(1.0f - poissonRatio) / 2.0f;

	D *= youngModulus / (1.0f - pow(poissonRatio, 2.0f));

#if defined (RECT) && defined (ANALYTICAL)
	g_D =
		1.0f,        	poissonRatio,	0.0f,
		poissonRatio,	1.0,         	0.0f,
		0.0f,        	0.0f,        	(1.0f - poissonRatio) / 2.0f;

	g_D = g_D.mul_scalar(youngModulus / (1.0f - pow(poissonRatio, 2.0f)));
	PrecalculateIntegrals();
#endif

	infile >> nodesCount;
	nodesX.resize(nodesCount);
	nodesY.resize(nodesCount);

	for (int i = 0; i < nodesCount; ++i)
	{
		infile >> nodesX[i] >> nodesY[i];
	}

	int elementCount;
	infile >> elementCount;

	for (int i = 0; i < elementCount; ++i)
	{
		Element element;
#ifdef RECT
		infile >> element.nodesIds[0] >> element.nodesIds[1] >> element.nodesIds[2] >> element.nodesIds[3];
#else
		infile >> element.nodesIds[0] >> element.nodesIds[1] >> element.nodesIds[2];
#endif
		elements.push_back(element);
	}

	int constraintCount;
	infile >> constraintCount;

	for (int i = 0; i < constraintCount; ++i)
	{
		Constraint constraint;
		int type;
		infile >> constraint.node >> type;
		constraint.type = static_cast<Constraint::Type>(type);
		constraints.push_back(constraint);
	}

	loads.resize(2 * nodesCount);
	loads.setZero();

	int loadsCount;
	infile >> loadsCount;

	for (int i = 0; i < loadsCount; ++i)
	{
		int node;
		float x, y;
		infile >> node >> x >> y;
		loads[2 * node + 0] = x;
		loads[2 * node + 1] = y;
	}

	std::vector<Eigen::Triplet<float> > triplets;
	for (std::vector<Element>::iterator it = elements.begin(); it != elements.end(); ++it)
	{
		it->CalculateStiffnessMatrix(D, triplets);
	}

	Eigen::SparseMatrix<float> globalK(2 * nodesCount, 2 * nodesCount);
	globalK.setFromTriplets(triplets.begin(), triplets.end());

	ApplyConstraints(globalK, constraints);

	Eigen::SimplicialLDLT<Eigen::SparseMatrix<float> > solver(globalK);

	Eigen::VectorXf displacements = solver.solve(loads);

	outfile << displacements << std::endl;

	for (std::vector<Element>::iterator it = elements.begin(); it != elements.end(); ++it)
	{
#ifdef RECT
		Eigen::Matrix<float, 8, 1> delta;
		delta << displacements.segment<2>(2 * it->nodesIds[0]),
		         displacements.segment<2>(2 * it->nodesIds[1]),
		         displacements.segment<2>(2 * it->nodesIds[2]),
		         displacements.segment<2>(2 * it->nodesIds[3]);
#else
		Eigen::Matrix<float, 6, 1> delta;
		delta << displacements.segment<2>(2 * it->nodesIds[0]),
		         displacements.segment<2>(2 * it->nodesIds[1]),
		         displacements.segment<2>(2 * it->nodesIds[2]);
#endif

#if defined (RECT) && defined (ANALYTICAL)
		// FIXME Now B is not constant in cell, how to estimate?
		outfile << 0.0f << std::endl;
#else
		Eigen::Vector3f sigma = D * it->B * delta;
		float sigma_mises = sqrt(sigma[0] * sigma[0] - sigma[0] * sigma[1] + sigma[1] * sigma[1] + 3.0f * sigma[2] * sigma[2]);

		outfile << sigma_mises << std::endl;
#endif
	}
	return 0;
}
