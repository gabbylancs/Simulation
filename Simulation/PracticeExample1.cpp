///* Just a little example of how to use Eigen's Levenberg-Marquardt algorithm to fit a non-linear model to some data.
//    We'll fit a simple y = ae^(bx) model to some synthetic data.
//*/
//
//#include <iostream>
//#include <vector>
//#include <Eigen/Dense>
//#include <unsupported/Eigen/NonLinearOptimization>
//#include <unsupported/Eigen/NumericalDiff> 
//
//
//struct MyFitter
//{
//    // 1. Mandatory Typedefs (NumericalDiff looks for these names)
//    typedef double Scalar;
//    enum { InputsAtCompileTime = 2, ValuesAtCompileTime = Eigen::Dynamic };
//    typedef Eigen::VectorXd InputType;  // Must be named 'InputType'
//    typedef Eigen::VectorXd ValueType;  // Must be named 'ValueType'
//    typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> JacobianType;
//
//    const Eigen::VectorXd& xData;
//    const Eigen::VectorXd& yData;
//
//    MyFitter(const Eigen::VectorXd& x, const Eigen::VectorXd& y)
//        : xData(x), yData(y) {
//    }
//
//    // 2. The Operator (Must return int and be const)
//    int operator()(const InputType& params, ValueType& fvec) const
//    {
//        double a = params[0];
//        double b = params[1];
//
//        for (int i = 0; i < xData.size(); ++i) {
//            fvec(i) = (a * std::exp(b * xData(i))) - yData(i);
//        }
//
//        return 0;
//    }
//
//    // 3. Jacobian (optional, but can speed up convergence if provided)
//    int df(const InputType& params, JacobianType& fjac) const
//    {
//        double a = params[0];
//        double b = params[1];
//
//        for (int i = 0; i < xData.size(); ++i)
//        {
//            double exp_bx = std::exp(b * xData(i));
//            fjac(i, 0) = exp_bx;                         // df/da
//            fjac(i, 1) = a * xData(i) * exp_bx;          // df/db
//        }
//
//        return 0;
//    }
//
//    // 4. Helper methods for matrix sizing
//    int inputs() const { return 2; }            // number of parameters (a, b)
//    int values() const { return xData.size(); } // number of residuals
//};
//
//
//int main()
//{
//    //1. Generate synthetic data -> y = 2.5 * exp(0.4 * x) + noise
//    Eigen::VectorXd x(5), y(5);
//    x << 0, 1, 2, 3, 4; // Independent variable
//    y << 2.5, 3.7, 5.5, 8.3, 12.4;
//
//    // 2. Initialize the fitter with initial guess for parameters (a, b)
//    Eigen::VectorXd params(2);
//    params << 2.0, 0.5; // Initial guess for a and b
//
//    //3. Set up the Levenberg-Marquardt solver
//    MyFitter fitter(x, y);
//    Eigen::LevenbergMarquardt<MyFitter> lm(fitter);
//
//    int info = lm.minimize(params);
//
//    std::cout << "Status: " << info << " (1-3 is success)" << std::endl;
//    std::cout << "Optimized a: " << params(0) << std::endl;
//    std::cout << "Optimized b: " << params(1) << std::endl;
//
//    return 0;
//
//}