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
//    // Total parameters: 1 (camera)
//    enum { InputsAtCompileTime = Eigen::Dynamic, ValuesAtCompileTime = Eigen::Dynamic }; // Number of parameters to be optimized and number of observations.
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
//        double delta = 0.5; // Threshold for Huber-like loss
//
//        // params[0] is the camera
//        double cam = params[0];
//        if (std::abs(cam) < 1e-6) cam = 1e-6; // Avoid division by zero
//
//        //loop through the observed pixels and compute the residuals
//        for (int i = 0; i < observed_pixels.size(); ++i)
//        {
//            double p = params[i + 1]; // Point position
//            double residual = p / cam - observed_pixels[i]; // Residual: predicted - observed
//
//            // Apply Huber-like weighting
//            if (std::abs(residual) <= delta)
//            {
//                fvec[i] = residual;
//            }
//            else
//            {
//                // Linear growth: sign(r) * sqrt(2 * delta * |r| - delta^2)
//                // Or more simply for your logic: 
//                fvec[i] = (residual > 0 ? 1 : -1) * std::sqrt(delta * std::abs(residual));
//            }
//        }
//
//        // The +1 residual: Anchor the camera to 1.0
//        // We can multiply this by a "weight" (e.g., 1000.0) to make it a hard constraint
//        fvec[observed_pixels.size()] = (cam - 1.0) * 1.0;
//        return 0;
//    }
//
//    // Need to work out what i would do if the pixel count was dynamic.
//    // Jacobian: How each residual changes w.r.t EACH parameter
//    //cols = number of resudials, rows = number of parameters
//    int df(const InputType& params, JacobianType& fjac) const
//    {
//        double cam = params[0];
//        if (std::abs(cam) < 1e-6) cam = 1e-6;
//
//        // First, set the entire Jacobian to zero
//        fjac.setZero();
//
//        for (int i = 0; i < observed_pixels.size(); ++i)
//        {
//            double p = params[i + 1];
//
//            // 1. Derivative w.r.t the camera (Column 0)
//            fjac(i, 0) = -p / (cam * cam);
//
//            // 2. Derivative w.r.t the point (Column i + 1)
//            fjac(i, i + 1) = 1.0 / cam;
//        }
//
//        // Derivative of the camera anchor residual
//        fjac(observed_pixels.size(), 0) = 1.0 * 1.0; // Derivative w.r.t camera
//
//        return 0;
//    }
//
//    // Number of parameters: 1 camera + N points
//    int inputs() const { return 1 + observed_pixels.size(); }
//
//    // Number of residuals: N observations
//    int values() const { return observed_pixels.size() + 1; }
//};
//
//
//int main()
//{
//    // 1. Generate synthetic observed pixel data for two points seen by the camera
//    Eigen::VectorXd observed_pixels(5);
//    observed_pixels << 0.5, 1.0, 1.2, 1.5, 2.0; // Simulated observed pixel positions for two points
//
//
//    // 2. Initialize the fittter with initial guess for parameters (camera position and point positions)
//    Eigen::VectorXd params(6);
//    params << 0.8, 0.3, 0.7, 0.1, 1.3, 7; //
//
//    // 3. Create the functor and the Levenberg-Marquardt solver
//    MiniBundleAdjustment functor(observed_pixels);
//    Eigen::LevenbergMarquardt<MiniBundleAdjustment> lm(functor);
//    int info = lm.minimize(params);
//
//    std::cout << "Status: " << info << std::endl;
//    std::cout << "Optimized Camera: " << params[0] << " (Target: 1.0)" << std::endl;
//    for (int i = 0; i < observed_pixels.size(); ++i) {
//        std::cout << "Point " << i << ": " << params[i + 1] << std::endl;
//    }
//
//    return 0;
//
//}
//
//
//
