#include <iostream>
#include <cv.h>
#include <highgui.h>
#include <cvaux.h>
#include <cmath>
#include <iomanip>

using namespace std;
using namespace cv;

void kerns(int size, Mat &x, Mat &y)
{
  float sigma = size/5.;
  // |x|=|y|=5*sigma
  x.create(1,size,CV_32FC1);
  y.create(size,1,CV_32FC1);

  // 0.39894228 = 1/sqrt(2*pi)
  for ( int i = 0; i < size; i++ )
    x.at<float>(0,i) = y.at<float>(i,0) =
      0.39894228/sigma*(exp(-(pow(i-(size-1)/2.,2)/(2*pow(sigma,2)))));
}

// returns subpixel values using linear interpolation
float getVal(const Mat& img, float row, float col)
{
  // zero because outside of bounds
  if ( row <= -1.0 || row >= img.rows || col <= -1.0 || col >= img.cols )
    return 0.;
  else if ( (int)row == row && (int)col == col )  // using whole numbers
    return img.at<float>((int)row,(int)col);
  else
  {
    // rather ugly linear interpolation
    static int lRow, hRow, lCol, hCol;
    static float q12, q22, q11, q21, rOff, cOff, r1, r2;
  
    if ( (int)row == row )
      lRow = hRow = (int)row;
    else
    {
      if ( row < 0 )
        lRow = (int)row - 1;
      else
        lRow = (int)row;
      hRow = lRow + 1;
    }

    if ( (int)col == col )
      lCol = hCol = (int)col;
    else
    {
      if ( col < 0 )
        lCol = (int)col-1;
      else
        lCol = (int)col;
      hCol = lCol + 1;
    }
    
    if ( lRow < 0 || lCol < 0 || lRow >= img.rows || lCol >= img.cols )
      q11 = 0;
    else
      q11 = img.at<float>(lRow,lCol);

    if ( hRow < 0 || lCol < 0 || hRow >= img.rows || lCol >= img.cols )
      q12 = 0;
    else
      q12 = img.at<float>(hRow,lCol);

    if ( lRow < 0 || hCol < 0 || lRow >= img.rows || hCol >= img.cols )
      q21 = 0;
    else
      q21 = img.at<float>(lRow,hCol);

    if ( hRow < 0 || hCol < 0 || hRow >= img.rows || hCol >= img.cols )
      q22 = 0;
    else
      q22 = img.at<float>(hRow,hCol);
 
    if ( hCol == lCol )
    {
      r1 = q11;
      r2 = q12;
    }
    else
    {
      r1 = (hCol - col)*q11 + (col - lCol)*q21;
      r2 = (hCol - col)*q12 + (col - lCol)*q22;
    }
    
    if ( hRow == lRow )
      return r1;
    else
      return (hRow - row)*r1 + (row - lRow)*r2;
  }

}

// assumes CV_32FC1 type Matricies
void filter(const Mat &input, Mat &output, const Mat &kern)
{
  int rows = input.rows,
      cols = input.cols,
      kernRows = kern.rows,
      kernCols = kern.cols;

  output.create(rows,cols,CV_32FC1);
  output = cvRealScalar(0.);

  int i, j, k, l;

  for ( i = 0; i < rows; i++ )
    for ( j = 0; j < cols; j++ )
      for ( k = 0; k < kernRows; k++ )
        for ( l = 0; l < kernCols; l++ )
          output.at<float>(i,j) += kern.at<float>(k,l)*getVal(input,i+k-(kernRows-1)/2.,j+l-(kernCols-1)/2.);
}

int main(int argc, char *argv[])
{
  if ( argc < 5 )
  {
    cout << "Command line arguments are as follows..." << endl
         << "<windowsize> <input> <output1> <output2>" << endl
         << "sigma must be an interger value" << endl
         << "output1 is the input blurred using seperable kernels" << endl
         << "output2 is the input blurred using OpenCV's GaussianBlur function" << endl;
    return -1;
  }

  float sigma = atoi(argv[1])/5.;

  Mat xKern,yKern,xGauss,smooth,show1,show2,cmp,
    img = imread(argv[2],0), fImg;

  // convert to floating point
  img.convertTo(fImg,CV_32FC1);

  // build kernels
  kerns(atoi(argv[1]),xKern,yKern);

  // filter image
  filter(fImg,xGauss,xKern);
  filter(xGauss,smooth,yKern);
  
  // create comparison image
  if ( (int)(5*sigma) % 2 != 0 )
    GaussianBlur(img,cmp,cvSize(5*sigma,5*sigma),sigma,sigma,BORDER_CONSTANT);
  else
    cmp = smooth;

  // display
  normalize(smooth,show1,0,255,CV_MINMAX,CV_8UC1);
  imshow("My Blur",show1);  

  normalize(cmp,show2,0,255,CV_MINMAX,CV_8UC1);
  imshow("OpenCV's Blur",show2);

  // save results
  imwrite(argv[3],show1);
  imwrite(argv[4],show2);

  waitKey(0);

  // calculate mean square error

  float mse = 0.0;

  for ( int i = 0; i < img.rows; i++ )
    for ( int j = 0; j < img.cols; j++ )
      mse += pow((float)(show1.at<uchar>(i,j))-(float)(show2.at<uchar>(i,j)),2);
  mse /= img.rows*img.cols;

  cout << "Mean Square Error: " << mse << endl;
  return 0;
}
