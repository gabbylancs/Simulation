///* Just a little example of how to use Eigen's Levenberg-Marquardt algorithm to fit a non-linear model to some data.
//   This time we'll do Mini-BA i.e. a mini bundle adjustment algorithm with only a few poses - camera and points.
//*/
//
//#include <iostream>
//#include <vector>
//#include <Eigen/Dense>
//#include <unsupported/Eigen/NonLinearOptimization>
//#include <unsupported/Eigen/NumericalDiff> 
//
//struct MiniBundleAdjustment
//{
//    typedef double Scalar;
//    // Total parameters: 1 (camera) + 2 (points) = 3
//    enum { InputsAtCompileTime = 3, ValuesAtCompileTime = Eigen::Dynamic }; // Number of parameters to be optimized and number of observations.
//    typedef Eigen::VectorXd InputType;
//    typedef Eigen::VectorXd ValueType;
//    typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> JacobianType;
//
//    // Observations: Where the points were seen in the image
//    const Eigen::VectorXd& observed_pixels;
//
//    MiniBundleAdjustment(const Eigen::VectorXd& obs) : observed_pixels(obs) {}
//
//
//    // we pass through the parameters (initial guess) and try to minimise the fvec. 
//    // Model: Pixel = Point_Pos / Camera_Pos (Simplified projection)
//    int operator()(const InputType& params, ValueType& fvec) const
//    {
//        double cam = params[0];
//        double p1 = params[1];
//        double p2 = params[2];
//
//        // Residual 1: Camera looking at Point 1
//        fvec(0) = (p1 / cam) - observed_pixels[0];
//        // Residual 2: Camera looking at Point 2
//        fvec(1) = (p2 / cam) - observed_pixels[1];
//
//        return 0;
//    }
//
//    // Need to work out what i would do if the pixel count was dynamic.
//    // Jacobian: How each residual changes w.r.t EACH parameter
//    //cols = number of resudials, rows = number of parameters
//    int df(const InputType& params, JacobianType& fjac) const
//    {
//        double cam = params[0];
//        double p1 = params[1];
//        double p2 = params[2];
//
//        // Row 0: Derivatives of fvec(0) w.r.t [cam, p1, p2]
//        fjac(0, 0) = -p1 / (cam * cam); // d/d_cam
//        fjac(0, 1) = 1.0 / cam;         // d/d_p1
//        fjac(0, 2) = 0.0;               // d/d_p2
//
//        // Row 1: Derivatives of fvec(1) w.r.t [cam, p1, p2]
//        fjac(1, 0) = -p2 / (cam * cam); // d/d_cam
//        fjac(1, 1) = 0.0;               // d/d_p1
//        fjac(1, 2) = 1.0 / cam;         // d/d_p2
//
//        return 0;
//    }
//
//    int inputs() const { return 3; } // [cam, p1, p2]
//    int values() const { return 2; } // 2 observations
//};
//
//
//int main()
//{
//    // 1. Generate synthetic observed pixel data for two points seen by the camera
//    Eigen::VectorXd observed_pixels(2);
//    observed_pixels << 0.5, 1.0; // Simulated observed pixel positions for two points
//
//
//    // 2. Initialize the fittter with initial guess for parameters (camera position and point positions)
//    Eigen::VectorXd params(3);
//    params << 1.0, 0.5, 1.0; //
//
//    // 3. Create the functor and the Levenberg-Marquardt solver
//    MiniBundleAdjustment functor(observed_pixels);
//    Eigen::LevenbergMarquardt<MiniBundleAdjustment> lm(functor);
//    int info = lm.minimize(params);
//
//    std::cout << "Status: " << info << " (1-3 is success)" << std::endl;
//    std::cout << "Optimized a: " << params(0) << std::endl;
//    std::cout << "Optimized b: " << params(1) << std::endl;
//
//    return 0;
//
//}
//
//
//
